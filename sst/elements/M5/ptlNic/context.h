#ifndef _context_h
#define _context_h

#include <deque>
#include <vector>
#include <list>
#include "ptlEvent.h"
#include "ptlHdr.h"
#include "callback.h"
#include "ptlNicTypes.h"
#include "cmdQueue.h"

class PtlNic;
class RecvEntry;
class Context {

    typedef int handle_t; 

    struct PT {
        bool used;
        int eq_handle;
        unsigned int options;
        std::list<int> meL[2];
    };
    
    struct GetSendEntry;

    struct TriggeredOP {
        union {
            GetSendEntry*   get;
        } u;
        ptl_size_t      count;        
    };

    struct CT {
        Addr                    vaddr;
        ptl_ct_event_t          event;
        std::list< TriggeredOP* >        triggered;
    };

    struct EQ {
        Addr                    vaddr;
        ptl_size_t              count;
        ptl_size_t              size;
    };

    struct ME {
        ptl_me_t                me;
        void*                   user_ptr;
        ptl_size_t              offset;
        bool                    used; // purely for sanity check
    };

    struct MD {
        ptl_md_t                md;
        bool                    used; // purely for sanity check
    };

  public:
    Context( PtlNic* nic, cmdContextInit_t& cmd );
    ~Context();

    // mapped to Portals API
    void NIInit( cmdPtlNIInit_t& ); 
    void NIFini( cmdPtlNIFini_t& ); 
    void MDBind( cmdPtlMDBind_t& );
    void MDRelease( cmdPtlMDRelease_t& );
    void MEAppend( cmdPtlMEAppend_t& );
    void MEUnlink( cmdPtlMEUnlink_t& );
    void CTAlloc( cmdPtlCTAlloc_t& );
    void CTFree( cmdPtlCTFree_t& );
    void EQAlloc( cmdPtlEQAlloc_t& );
    void EQFree( cmdPtlEQFree_t& );
    void Put( cmdPtlPut_t& );
    void Get( cmdPtlGet_t& );
    void TrigGet( cmdPtlTrigGet_t& );
    void PTAlloc( cmdPtlPTAlloc_t& );
    void PTFree( cmdPtlPTFree_t& );

    RecvEntry* processHdrPkt( void* pkt );
    ptl_pid_t pid() { return m_pid; }

 private:
    void initPid( ptl_pid_t pid );
    void initOptions( int options );
    void initId();

    void addCT( int handle, ptl_size_t value );
    ptl_ct_event_t* findCTEvent( int handle );
    Addr findCTAddr( int handle );

    ptl_me_t* findME( int handle );

    struct EQ&  findEQ( int handle );
    Addr findEventAddr( int handle, int pos );

  private:
    void freeHostMEHandle( int );
    int  search( ptl_nid_t nid, PtlHdr& hdr, ptl_list_t );

    struct OverflowEntry;

    struct XXX;

    void doMEOverflow( XXX* );
    XXX* searchOverflow( ptl_me_t& );

    RecvEntry* processHdrPkt( ptl_nid_t nid, PtlHdr* hdr );
    RecvEntry* processMatch( ptl_nid_t, PtlHdr*, int me_handle, ptl_list_t );
    RecvEntry* processPut( ptl_nid_t, PtlHdr*, int me_handle, ptl_list_t );
    RecvEntry* processGet( ptl_nid_t, PtlHdr*, int me_handle, ptl_list_t );
    void processAck( PtlHdr * );
    RecvEntry* processReply( PtlHdr * );
    void recvFini( XXX* );

    void writeEvent( int eq_handle,
                    ptl_event_kind_t    type,
                    ptl_process_t&      initiator,
                    ptl_pt_index_t      pt_index,
                    ptl_uid_t           uid,
                    ptl_jid_t           jid,
                    ptl_match_bits_t    match_bits,
                    ptl_size_t          rlength,
                    ptl_size_t          mlength,
                    ptl_size_t          remote_offset,
                    void *              start,
                    void *              user_ptr,
                    ptl_hdr_data_t      hdr_data,
                    ptl_ni_fail_t       ni_fail_type );

    void writeCtEvent( int ct_handle, ptl_ct_event_t&  );
    
    void doTriggered( int handle );

    void writeReplyEvent( int eq_handle,
                    ptl_size_t          mlength,
                    ptl_size_t          remote_offset,
                    void *              user_ptr,
                    ptl_ni_fail_t       ni_fail_type );

    void writeAckEvent( int eq_handle,
                    ptl_size_t          mlength,
                    ptl_size_t          remote_offset,
                    void *              user_ptr,
                    ptl_ni_fail_t       ni_fail_type );

    void writeSendEvent( int eq_handle,
                    void *              user_ptr,
                    ptl_ni_fail_t       ni_fail_type );


    struct YYY {
        PtlHdr      hdr;
        void       *user_ptr;
        int         md_handle; 
        CallbackBase* callback;
    };

    //*************
    struct PutSendEntry : YYY {
        enum { WaitSend, WaitAck } state;
    };
    typedef Callback< Context, PutSendEntry >  PutCallback;
    bool putCallback( PutSendEntry* );

    //*************
    struct GetSendEntry : YYY {
        ptl_size_t  local_offset;
        ptl_nid_t   destNid;
    };
    typedef Callback< Context, GetSendEntry >  GetCallback;
    bool getCallback( GetSendEntry* );

    struct XXX {
        ptl_nid_t       srcNid;
        int             me_handle;
        ptl_size_t      mlength;
        void*           start; 
        CallbackBase*   callback;
        int             op;
        ptl_list_t      list;
        PtlHdr          origHdr;
        bool            unlink;
        cmdPtlMEAppend_t* cmd;
    };

    struct PutRecvEntry : XXX {
        PtlHdr        ackHdr;
        enum { WaitRecvComp, WaitAckSent } state;
    };

    typedef Callback<Context,PutRecvEntry>  PutRecvCallback;
    bool putRecvCallback( PutRecvEntry* );

    //*************
    struct GetRecvEntry : XXX {
        PtlHdr        replyHdr;
    };
    typedef Callback< Context, GetRecvEntry >  GetRecvCallback;
    bool getRecvCallback( GetRecvEntry* );

    //*************
    struct EventEntry {
        EventEntry( int _handle ) : handle( _handle ) {}
        struct PtlEventInternal event;
        ptl_ct_event_t          ctEvent;
        CallbackBase*           callback;
        int handle;
    };
    typedef Callback<Context,EventEntry>  EventCallback;
    bool eventCallback( EventEntry* );
    void writeEvent( EventEntry* );
    void writeCtEvent( EventEntry* );

    int allocKey();
    void freeKey( int );

    bool                    m_logicalIF;
    bool                    m_matching; 

    bool checkME( ptl_nid_t, PtlHdr& hdr, ptl_me_t& );

    typedef std::deque<PutRecvEntry*>  overflowHdrList_t;
    overflowHdrList_t                   m_overflowHdrList;

    ptl_process_t           m_id;
    ptl_pid_t               m_pid;
    ptl_uid_t               m_uid;
    ptl_jid_t               m_jid;

    std::vector<PT>         m_ptV;
    std::vector<CT>         m_ctV;
    std::vector<EQ>         m_eqV;
    std::vector<MD>         m_mdV;
    std::vector<ME>         m_meV;
    ptl_ni_limits_t         m_limits;
    PtlNic*                 m_nic;
    std::map< int, PutSendEntry* >   m_putM;
    std::map< int, GetSendEntry* >   m_getM;

    int*                    m_meUnlinkedHostPtr;
    // instead of dynamically allocating/freeing a buffer for each write to
    // host use m_meUnlinked as local buffer
    int                     m_meUnlinked[ ME_UNLINKED_SIZE ];
    int                     m_meUnlinkedPos;

    std::deque<int>         m_keys;                     
};

inline int Context::allocKey()
{ 
    assert( ! m_keys.empty() );
    int tmp = m_keys.front();
    m_keys.pop_front();
    return tmp;
}

inline void Context::freeKey(int key)
{
    m_keys.push_back(key);
} 

inline void Context::writeCtEvent( int ct_handle, ptl_ct_event_t& event )
{
    EventEntry* entry = new EventEntry( ct_handle );
    assert( entry );

    entry->callback = new EventCallback( this, &Context::eventCallback, entry );
    entry->ctEvent = event;

    writeCtEvent( entry );
}

inline void Context::writeEvent( int eq_handle,
                    ptl_event_kind_t    type,
                    ptl_process_t&      initiator,
                    ptl_pt_index_t      pt_index,
                    ptl_uid_t           uid,
                    ptl_jid_t           jid,
                    ptl_match_bits_t    match_bits,
                    ptl_size_t          rlength,
                    ptl_size_t          mlength,
                    ptl_size_t          remote_offset,
                    void *              start,
                    void *              user_ptr,
                    ptl_hdr_data_t      hdr_data,
                    ptl_ni_fail_t       ni_fail_type )
{
    EventEntry* entry = new EventEntry( eq_handle );
    assert( entry );

    entry->callback = new EventCallback( this, &Context::eventCallback, entry );

    entry->event.user.type          = type;
    entry->event.user.initiator     = initiator;
    entry->event.user.pt_index      = pt_index;
    entry->event.user.uid           = uid;
    entry->event.user.jid           = jid;
    entry->event.user.match_bits    = match_bits;
    entry->event.user.rlength       = rlength;
    entry->event.user.mlength       = mlength;
    entry->event.user.remote_offset = remote_offset;
    entry->event.user.start         = start;
    entry->event.user.user_ptr      = user_ptr;
    entry->event.user.hdr_data      = hdr_data;
    entry->event.user.ni_fail_type  = ni_fail_type;
//    entry->event.user.atomic_operation = atomic_operation;
//    entry->event.user.atomic_type      = atomic_type;

    writeEvent( entry );
}

inline void Context::writeReplyEvent( int eq_handle,
                    ptl_size_t          mlength,
                    ptl_size_t          remote_offset,
                    void *              user_ptr,
                    ptl_ni_fail_t       ni_fail_type )
{
    ptl_process_t initiator;
    initiator.phys.nid = initiator.phys.pid = -1;
    
    writeEvent( eq_handle,
                PTL_EVENT_REPLY,
                initiator,
                0,
                0,
                0,
                0,
                0,
                mlength,
                remote_offset,
                0,
                user_ptr,
                0,
                ni_fail_type
            );
}

inline void Context::writeAckEvent( int eq_handle,
                    ptl_size_t          mlength,
                    ptl_size_t          remote_offset,
                    void *              user_ptr,
                    ptl_ni_fail_t       ni_fail_type )
{
    ptl_process_t initiator;
    initiator.phys.nid = initiator.phys.pid = -1;

    writeEvent( eq_handle,
                PTL_EVENT_ACK,
                initiator,
                0,
                0,
                0,
                0,
                0,
                mlength,
                remote_offset,
                0,
                user_ptr,
                0,
                ni_fail_type
            );
}

inline void Context::writeSendEvent( int eq_handle,
                    void *              user_ptr,
                    ptl_ni_fail_t       ni_fail_type )
{
    ptl_process_t initiator;
    initiator.phys.nid = initiator.phys.pid = -1;

    writeEvent( eq_handle,
                PTL_EVENT_SEND,
                initiator,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                user_ptr,
                0,
                ni_fail_type
            );
}

#endif
