// Copyright 2009-2019 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2019, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

class LoadUnit : public Unit {
	struct Entry { 
		Entry( MemReq* req, Callback* callback, SimTime_t time ) : req(req), callback(callback), postTime(time) {}
		MemReq* req;
		Callback* callback;
		SimTime_t postTime; 
	};

	std::string m_name;

  public:

    SST_ELI_REGISTER_SUBCOMPONENT_API(SimpleMemoryModel::LoadUnit, SimpleMemoryModel*, Output*, int, int, Unit*, int, std::string )
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        LoadUnit,
        "firefly",
        "simpleMemory.loadUnit",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "",
        SimpleMemoryModel::LoadUnit
    )

    LoadUnit( Component* comp, Params& ) : Unit( comp, NULL, NULL) {}
    LoadUnit( ComponentId_t compId, Params& ) : Unit( compId, NULL, NULL) {}

    LoadUnit( Component* comp, Params&, SimpleMemoryModel*, Output*, int, int, Unit*, int, std::string ) :
        Unit( comp, NULL, NULL) {}

    LoadUnit( ComponentId_t compId, Params&, SimpleMemoryModel* model, Output* dbg, int id, int thread_id, Unit* cache, int numSlots, std::string name ) :
        Unit( compId, model, dbg ),  m_qSize(numSlots), m_cache(cache),  m_blocked(false), m_scheduled(false), 
			m_blockedSrc(NULL) , m_numPending(0)//, m_name(name)
	{
		std::stringstream tmp;
		tmp << "_" << name << "_" << id << "_"<< thread_id;
		m_name = tmp.str();

        m_prefix = "@t:" + std::to_string(id) + ":SimpleMemoryModel::" + name + "LoadUnit::@p():@l ";
        dbg->verbosePrefix(prefix(),CALL_INFO,1,LOAD_MASK,"maxPending=%d\n",m_qSize);
		m_pendingQdepth = model->registerStatistic<uint64_t>(name + "_load_pending_Q_depth",std::to_string(thread_id));
		m_latency = model->registerStatistic<uint64_t>(name + "_load_latency",std::to_string(thread_id));
    }


	std::string& name() { return m_name; }

    bool load( UnitBase* src, MemReq* req, Callback* callback ) {

        dbg().verbosePrefix(prefix(),CALL_INFO,1,LOAD_MASK,"addr=%#" PRIx64 " length=%lu pending=%lu\n",req->addr, req->length, m_pendingQ.size() );

		m_pendingQ.push( Entry( req, callback, model().getCurrentSimTimeNano() ) );
		++m_numPending; 
		m_pendingQdepth->addData( m_numPending );

        if ( m_numPending <= m_qSize ) {
            if ( ! m_blocked && ! m_scheduled ) {
				Callback* cb = new Callback;
				*cb = std::bind( &LoadUnit::process, this );
                model().schedCallback( 0, cb );
                m_scheduled = true;
            }
		}

       	if ( m_numPending == m_qSize  ) {
			dbg().verbosePrefix(prefix(),CALL_INFO,2,LOAD_MASK,"blocking src\n");
            m_blockedSrc = src;
            return true;
        } else {
            return false;
        }
    }

  private:
	void process() {
		assert( ! m_pendingQ.empty() );
        Entry& entry = m_pendingQ.front();
     	dbg().verbosePrefix(prefix(),CALL_INFO,3,LOAD_MASK,"addr=%#" PRIx64 " length=%lu pending=%lu\n",entry.req->addr,entry.req->length,m_pendingQ.size() );

        assert( m_blocked == false );
        m_scheduled = false;

        SimTime_t issueTime = model().getCurrentSimTimeNano();

		Hermes::Vaddr addr = entry.req->addr;
		size_t length = entry.req->length;
		SimTime_t postTime = entry.postTime;
		Callback* cb = new Callback;
		*cb = [=]() {

				SimTime_t currentTime = model().getCurrentSimTimeNano();
                SimTime_t latency = currentTime - issueTime;

				if ( currentTime - postTime ) {
					m_latency->addData( currentTime - postTime );
				}
        		dbg().verbosePrefix(prefix(),CALL_INFO_LAMBDA,"process",1,LOAD_MASK," complete, latency=%" PRIu64 " addr=%#" PRIx64 " length=%lu pending=%lu\n",
													latency,addr,length,m_pendingQ.size() );

				--m_numPending;
				if ( entry.callback ) {
					dbg().verbosePrefix(prefix(),CALL_INFO_LAMBDA,"process",3,LOAD_MASK,"tell src load is complete\n");
					model().schedCallback( 0, entry.callback );
				}

        		if ( m_blockedSrc ) {
					dbg().verbosePrefix(prefix(),CALL_INFO_LAMBDA,"process",2,LOAD_MASK,"unblock src\n");
					model().schedResume( 0, m_blockedSrc, this );
            		m_blockedSrc = NULL;
        		}

        		dbg().verbosePrefix(prefix(),CALL_INFO_LAMBDA,"process",3,LOAD_MASK,"%s\n",m_blocked? "blocked" : "not blocked");

        		if ( ! m_blocked && ! m_scheduled && ! m_pendingQ.empty() ) {
					Callback* cb = new Callback;
					*cb = std::bind( &LoadUnit::process, this );
            		model().schedCallback( 0, cb );
            		m_scheduled = true;
        		}
			};
       	m_blocked = m_cache->load( this, entry.req, cb );
        dbg().verbosePrefix(prefix(),CALL_INFO,3,LOAD_MASK,"%s\n",m_blocked? "blocked" : " not blocked");
		assert( ! m_pendingQ.empty() );
       	m_pendingQ.pop();
	}

    void resume( UnitBase* src = NULL ) {
        dbg().verbosePrefix(prefix(),CALL_INFO,3,LOAD_MASK,"pending=%lu\n",m_pendingQ.size());

        assert( m_blocked == true );
        m_blocked = false;
        if ( ! m_scheduled && ! m_pendingQ.empty() ) {
            process();
        }
    }

	int m_numPending;
	bool m_scheduled;
	bool m_blocked;
	UnitBase* m_blockedSrc;


    Unit*  m_cache;
    std::queue<Entry> m_pendingQ;
    int m_qSize;
	Statistic<uint64_t>* m_pendingQdepth;
	Statistic<uint64_t>* m_latency;
};
