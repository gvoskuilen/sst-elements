/* SPIM S20 MIPS simulator.
   Declarations of registers and code for accessing them.
   Copyright (C) 1990-1994 by James Larus (larus@cs.wisc.edu).
   ALL RIGHTS RESERVED.

   SPIM is distributed under the following conditions:

     You may make copies of SPIM for your own use and modify those copies.

     All copies of SPIM must retain my name and copyright notice.

     You may not sell SPIM or distributed SPIM in conjunction with a
     commerical product or service without the expressed written consent of
     James Larus.

   THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
   IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
   PURPOSE. */


/* $Header: /var/home/larus/Software/larus/SPIM/RCS/reg.h,v 3.9 1994/06/03 21:49:07 larus Exp $
*/

#ifndef _REG_H
#define _REG_H

#include<list>

#if 0
// original reg word
typedef int32_t reg_word;
#else
// fault tracking version

namespace faultTrack {  
    typedef enum {FAULTED, 
                  CORRECTED_MATH,
                  CORRECTED_MEM,
                  CORRECTED_TMR, CORRECTED_SQUASH, 
                  READ_ADDR_ERROR, READ_DATA_ERROR,
                  WRITE_ADDR_ERROR, // caused a write address to be wrong
                  WRITE_DATA_ERROR,

                  WB_ERROR, // we write a faulted value to the RF
                  WB_ADDR_ERROR, // when we write a value back to the
                                 // wrong place in the RF the
                                 // wrongly-written is counted as a
                                 // FAULTED, the register which was
                                 // NOT written is a WB_ADDR_ERROR
                  WB_ADDR_NO_ERROR, // when we write a value back to
                                    // the wrong place, but the
                                    // unwritten value is not wrong,
                                    // we record this
                  INST_ERROR, // we executed & wrote back an incorrect
                              // instruction

                  LAST_FAULT_STATUS
    } faultStatus_t;

    typedef enum {NO_FAULT, // should not be used
                  WRONG_ADDR_WRITTEN, // we wrote data the wrong place
                  WRONG_ADDR_NOT_WRITTEN, // we didn't write data
                                          // where we should have
                  RIGHT_ADDR_WRONG_DATA} memFaults_t;

    /* Remove? switch over everything to fault_checker_t::location_idx?*/
    typedef enum {
        NO_LOC_FAULT = 0x0,
        RF_FAULT = 0x1, // at random cycle, flip a random bit in a
                        // random register
        ID_FAULT = 0x2,
        MDU_FAULT = 0x4, // Multiply-divide: at a random mult/div
                         // instruction, flip a bit(s) in the
                         // output of the Mult/Div unit
        MEM_PRE_ADDR_FAULT = 0x8, // Memory Stage "pre": at a random
                             // load/store, flip a  bit(s) in the
                             // address
        MEM_POST_FAULT = 0x10, // Memory Stage "post": at a random
                               // load/store, flip a  bit(s) in the
                               // output of the memory stage
        WB_FAULT = 0x20, //Writeback: at a random cycle, flip a random
                         //bit in a random value being written back
        ALU_FAULT = 0x40, // non-mult/div ALU error: at random cycle,
                          // fault ALU output
        MEM_BP_FAULT = 0x80, // fault in memory bypass. flip random
                             // bit(s) in bypass value
        CONTROL_FAULT = 0x100, // control or exception fault - crash for now
        INST_ADDR_FAULT = 0x200, // Flip random bit(s) in fetched
                                     // instruction address
        INST_TYPE_FAULT = 0x400, // Flip random bit(s) in
                                     // instruction before decdoe
        WB_ADDR_FAULT = 0x800, // writeback correct data to wrong register
        MEM_PRE_DATA_FAULT = 0x1000, // Memory Stage "pre": at a random
                                     // load/store, flip a  bit(s) in the data
        PC_FAULT = 0x2000 // corrupt the program counter
    } location_t; 
}

struct faultDesc {    
    faultTrack::location_t where;
    SST::Cycle_t when;
    SST::Cycle_t whenCorrected;
    
    int bits;
    faultTrack::faultStatus_t status;

    // for injecting
    faultDesc(faultTrack::location_t _where, int _bits) 
        : where(_where), when(-1), whenCorrected(0), bits(_bits), 
          status(faultTrack::FAULTED)
    {;}

    faultDesc(SST::Cycle_t _when, faultTrack::faultStatus_t _stat)
        : where(faultTrack::NO_LOC_FAULT), when(_when), whenCorrected(0), bits(0), status(_stat)
    {;}
};

struct memFaultDesc {    
    faultTrack::memFaults_t type;

    std::list<faultDesc> origFaults;

    memFaultDesc() : type(faultTrack::NO_FAULT) {;}
    memFaultDesc(faultTrack::memFaults_t _type, 
                 const std::list<faultDesc> oFaults)
        : type(_type), origFaults(oFaults) {;}
};


class reg_word {
    int32_t origData; // the 'correct' data
    int32_t data;    // data after potential faults

    typedef std::list<faultDesc> faultList_t;
    faultList_t faults;
#warning should move from being statics to structure created by proc core
    static SST::Cycle_t now;
    static uint64_t faultStats[faultTrack::LAST_FAULT_STATUS];
    static std::map<int32_t, memFaultDesc> memFaults;
    static std::map<int32_t, uint8_t> origMem;
    
    faultList_t findFaults() const {
        faultList_t list;
        for (auto &&i : faults) {
            if(i.status == faultTrack::FAULTED) {
                list.push_back(i);
            }
        }
        return list;
    }

    bool checkMemCorrect() {
        using namespace faultTrack;

        if (!faults.empty()) {
            if (data == origData) {
                for (auto &&i : faults) {
                    if(i.status == FAULTED) {
                        i.status = CORRECTED_MEM;
                        faultStats[CORRECTED_MEM]++;
                        i.whenCorrected = now;
                    }
                }
                return true;
            }
        }
        return false;
    }

    void checkMathCorrect() {
        using namespace faultTrack;

        if (!faults.empty()) {
            if (data == origData) {
                for (auto &&i : faults) {
                    if(i.status == FAULTED) {
                        i.status = CORRECTED_MATH;
                            faultStats[CORRECTED_MATH]++;
                            i.whenCorrected = now;
                            //printf("PRINT Math Correct\n");
                    }
                }
            }
        }
    }

    void assertOrig(const int32_t in, const int32_t addr, const size_t sz){
        // make sure input data and the data in origMem match
        // note: this can break when there is a mis-aligned store
        
        int32_t data = 0; // origData Data
        for (int i = sz-1; i >= 0; --i) {
            data <<= 8;
            data |= origMem[addr + i];
        }

        if (data != in) {
            printf("SHADOW MISMATCH data:%x in:%x addr:%x\n", data, in, addr);
        }

        //assert(data == in);
    }

    int32_t origMemRead(const int32_t addr, const size_t sz) {
        int32_t data = 0; // origData Data
        for (int i = sz-1; i >= 0; --i) {
            data <<= 8;
            auto iter = origMem.find(addr + i);
#warning make sure hadling if address is wrong
            //assert(iter != origMem.end()); // data should be in map?
            data |= iter->second;
        }

        return data;
    }

    static void origMemWrite(const int32_t addr, int32_t val, size_t sz) {
        for (int i = 0; i < sz; ++i) {
            origMem[addr+i] = val & 0xff;
            val >>= 8;
        }
    }

public:
    // at top of cycle, set the current time for record keeping
    static void setNow(SST::Cycle_t n) {
        now = n;
    }

    static SST::Cycle_t getNow() {
        return now;
    }

    static void countWBFaults(const reg_word &value) {
        if (value.data != value.origData) {
            faultStats[faultTrack::WB_ERROR]++;
        }
    }

    static void countINSTFaults(const reg_word &pc, const bool instF) {
        // If the pc.data is corrupted, it was loaded from the wrong
        // location (INST_ADDR_FAULT, PC_FAULT). If the instF is true,
        // then the decode was corrupted (INST_TYPE_FAULT)
        if (pc.data != pc.origData || instF) {
            faultStats[faultTrack::INST_ERROR]++;
        }
    }

    // print stats at end
    static void printStats() {
        printf("Fault Stats:\n");
#define PF(STR) printf("%s : %llu\n", #STR, faultStats[faultTrack::STR]);

        PF(FAULTED);
        PF(CORRECTED_MATH);
        PF(CORRECTED_MEM);
        PF(CORRECTED_TMR);
        PF(CORRECTED_SQUASH);
        PF(READ_ADDR_ERROR);
        PF(READ_DATA_ERROR);
        PF(WB_ERROR);
        PF(WB_ADDR_ERROR);
        PF(WB_ADDR_NO_ERROR);
        PF(INST_ERROR);
        PF(WRITE_ADDR_ERROR);
        PF(WRITE_DATA_ERROR);

#undef PF
    }

    static void initOrigMem(const mem_addr &addr, uint8_t data) {
        origMem[addr] = data;
    }

    // default constructor
    reg_word() {
        data = origData = 0;
        faults.clear();
    }

    // conversion constructor
    reg_word(const int &v) {
        data = origData = v;
        faults.clear();
    }

    // copy operator
    reg_word& operator= (const reg_word &o) {
        // Check for self assignment 
        if(this != &o) {
            origData = o.origData;
            data = o.data;
            if (o.faults.size() < 20) { // only bother with first 20 faults
                faults = o.faults;  
            } else {
                faults.clear();
                auto oi = o.faults.begin();
                for(int i = 0; i < 20; ++i) {
                    faults.push_back(*oi);
                }
            }
            // check for squashes?
        }

        return *this;
    }

    int32_t getData() const {
        return data;
    }

    int32_t getOrigData() const {
        return origData;
    }
    
    // reg_word / int

    // compount assignment operator
#define MAKE_RW_INT_COMP_OP(OPNM,OP)            \
    reg_word& OPNM(const int32_t &rhs) {        \
    data OP rhs;                                \
    origData OP rhs;                            \
    checkMathCorrect();                         \
    return *this;                               \
    }

    MAKE_RW_INT_COMP_OP(operator|=,|=);
    MAKE_RW_INT_COMP_OP(operator&=,&=);
    MAKE_RW_INT_COMP_OP(operator+=,+=);
    MAKE_RW_INT_COMP_OP(operator-=,-=);


#define MAKE_RW_INT_COMPARE_OP(OPNM,OP)                            \
    friend bool OPNM(const reg_word& lhs, const int& rhs){      \
    /* check for logic faults/errors check here?  */            \
        return lhs.data OP rhs;                                 \
    }

    MAKE_RW_INT_COMPARE_OP(operator>=,>=);
    MAKE_RW_INT_COMPARE_OP(operator==,==);
    MAKE_RW_INT_COMPARE_OP(operator!=,!=);
    MAKE_RW_INT_COMPARE_OP(operator<,<);


#define MAKE_RW_INT_OP(OPNM,OP) reg_word OPNM(const int &rhs) const { \
    reg_word newWord;                                                 \
    newWord.data = data OP rhs;                                       \
    newWord.origData = origData OP rhs;                               \
    newWord.faults.insert(newWord.faults.end(),                       \
                          newWord.faults.begin(),                     \
                          newWord.faults.end());                      \
    newWord.checkMathCorrect();                                       \
    return newWord;                                                   \
    }

    MAKE_RW_INT_OP(operator>>,>>);
    MAKE_RW_INT_OP(operator<<,<<);
    MAKE_RW_INT_OP(operator&,&);

    // return a reg_word whose value has been truncated to 16 bits and
    // sign extended
    reg_word truncExtend() const {
        reg_word newWord;
        newWord.data = (short)data;
        newWord.origData = (short)origData;
        newWord.faults.insert(newWord.faults.end(),  
                              faults.begin(),   
                              faults.end()); 
        newWord.checkMathCorrect();
        return newWord;
    }


    // reg_word / reg_word

#define MAKE_RW_RW_COMPARE_OP(OPNM,OP)                                  \
    friend bool OPNM(const reg_word& lhs, const reg_word& rhs){         \
    /* check for logic faults/errors check here? */                     \
        return lhs.data OP rhs.data;                                    \
    }

    MAKE_RW_RW_COMPARE_OP(operator==,==);
    MAKE_RW_RW_COMPARE_OP(operator!=,!=);
    MAKE_RW_RW_COMPARE_OP(operator<,<);

    friend reg_word setLessThan(const reg_word &lhs, const reg_word &rhs, 
                                bool unsignedComp = 0) {
        reg_word newWord; 

        if (unsignedComp) {
            newWord.data = ((uint64_t)lhs.data < (uint64_t)rhs.data) ? 1 : 0;
            newWord.origData = 
                ((uint64_t)lhs.origData < (uint64_t)rhs.origData) ? 1 : 0;
        } else {
            newWord.data = (lhs.data < rhs.data) ? 1 : 0;
            newWord.origData = (lhs.origData < rhs.origData) ? 1 : 0;
        }

        newWord.faults.insert(newWord.faults.end(),                    
                              lhs.faults.begin(),                          
                              lhs.faults.end());                           
        newWord.faults.insert(newWord.faults.end(),                    
                              rhs.faults.begin(),                      
                              rhs.faults.end());                       
        newWord.checkMathCorrect();     
        return newWord;                                                                                             
    }

    reg_word shift_right_logical(const reg_word &shamt) const {
        reg_word newWord;                                              
        newWord.data = ((uint32_t)data) >> shamt.data;
        newWord.origData = ((uint32_t)origData) >> shamt.origData;
        newWord.faults.insert(newWord.faults.end(),                    
                              faults.begin(),                          
                              faults.end());                           
        newWord.faults.insert(newWord.faults.end(),                    
                              shamt.faults.begin(),                      
                              shamt.faults.end());                       
        newWord.checkMathCorrect();                                    
        return newWord;    
    }

    reg_word unsigned_mod(const reg_word &rhs) const {
        reg_word newWord;                                              
        newWord.data = (uint64_t)data % (uint64_t)rhs.data;      
        newWord.origData = (uint64_t)origData % (uint64_t)rhs.origData;  
        newWord.faults.insert(newWord.faults.end(),                    
                              faults.begin(),                          
                              faults.end());                           
        newWord.faults.insert(newWord.faults.end(),                    
                              rhs.faults.begin(),                      
                              rhs.faults.end());                       
        newWord.checkMathCorrect();                                    
        return newWord;                                                
    }

    reg_word unsigned_div(const reg_word &rhs) const {
        reg_word newWord;                                              
        newWord.data = (uint64_t)data / (uint64_t)rhs.data;      
        newWord.origData = (uint64_t)origData / (uint64_t)rhs.origData;  
        newWord.faults.insert(newWord.faults.end(),                    
                              faults.begin(),                          
                              faults.end());                           
        newWord.faults.insert(newWord.faults.end(),                    
                              rhs.faults.begin(),                      
                              rhs.faults.end());                       
        newWord.checkMathCorrect();                                    
        return newWord;                                                
    }

    friend void long_multiply (const reg_word &v1, const reg_word &v2, 
                               reg_word *hi, reg_word *lo)
    {
#define SIGN_BIT(X) ((X) & 0x80000000)

#define ARITH_OVFL(RESULT, OP1, OP2) (SIGN_BIT (OP1) == SIGN_BIT (OP2) \
                                      && SIGN_BIT (OP1) != SIGN_BIT (RESULT))
        
        // copy faults
        lo->faults.insert(lo->faults.end(),
                          v1.faults.begin(),
                          v1.faults.end());
        lo->faults.insert(lo->faults.end(),
                          v2.faults.begin(),
                          v2.faults.end());
        hi->faults.insert(hi->faults.end(),
                          v1.faults.begin(),
                          v1.faults.end());
        hi->faults.insert(hi->faults.end(),
                          v2.faults.begin(),
                          v2.faults.end());


        long a, b, c, d;
        long bd, ad, cb, ac;
        long mid, mid2, carry_mid = 0;
        
        // do the multiplication on the data
        a = (v1.data >> 16) & 0xffff;
        b = v1.data & 0xffff;
        c = (v2.data >> 16) & 0xffff;
        d = v2.data & 0xffff;
        
        bd = b * d;
        ad = a * d;
        cb = c * b;
        ac = a * c;
        
        mid = ad + cb;
        if (ARITH_OVFL (mid, ad, cb))
            carry_mid = 1;
        
        mid2 = mid + ((bd >> 16) & 0xffff);
        if (ARITH_OVFL (mid2, mid, ((bd >> 16) & 0xffff)))
            carry_mid += 1;
        
        lo->data = (bd & 0xffff) | ((mid2 & 0xffff) << 16);
        hi->data = ac + (carry_mid << 16) + ((mid2 >> 16) & 0xffff);

        // do the multiplication on the origData
        if (v1.data != v1.origData || v2.data != v2.origData) {
            a = (v1.origData >> 16) & 0xffff;
            b = v1.origData & 0xffff;
            c = (v2.origData >> 16) & 0xffff;
            d = v2.origData & 0xffff;
            
            bd = b * d;
            ad = a * d;
            cb = c * b;
            ac = a * c;
            
            mid = ad + cb;
            if (ARITH_OVFL (mid, ad, cb))
                carry_mid = 1;
            
            mid2 = mid + ((bd >> 16) & 0xffff);
            if (ARITH_OVFL (mid2, mid, ((bd >> 16) & 0xffff)))
                carry_mid += 1;
            
            lo->origData = (bd & 0xffff) | ((mid2 & 0xffff) << 16);
            hi->origData = ac + (carry_mid << 16) + ((mid2 >> 16) & 0xffff);
        } else {
            lo->origData = lo->data;
            hi->origData = hi->data;
        }

        // corret if needed
        lo->checkMathCorrect();
        hi->checkMathCorrect();
    }


#define MAKE_RW_RW_OP(OPNM,OP) reg_word OPNM(const reg_word &rhs) const { \
        reg_word newWord;                                               \
        /* perform the operation */                                     \
        newWord.data = data OP rhs.data;                                \
        newWord.origData = origData OP rhs.origData;                    \
        if (!faults.empty() || !rhs.faults.empty()) {                   \
            if (0) printf("d: %x = %x # %x\n", newWord.data, data, rhs.data); \
            if (0) printf("o: %x = %x # %x\n", newWord.origData, origData, rhs.origData); \
        }                                                               \
        /* preserve the history */                                      \
        newWord.faults.insert(newWord.faults.end(),                     \
                              faults.begin(),                           \
                              faults.end());                            \
        newWord.faults.insert(newWord.faults.end(),                     \
                              rhs.faults.begin(),                       \
                              rhs.faults.end());                        \
        /* Check for math corrects */                                   \
        newWord.checkMathCorrect();                                     \
        return newWord;                                                 \
    }

    MAKE_RW_RW_OP(operator<<,<<);
    MAKE_RW_RW_OP(operator>>,>>);
    MAKE_RW_RW_OP(operator+,+);
    MAKE_RW_RW_OP(operator-,-);
    MAKE_RW_RW_OP(operator&,&);
    MAKE_RW_RW_OP(operator%,%);
    MAKE_RW_RW_OP(operator/,/);
    MAKE_RW_RW_OP(operator^,^);
    MAKE_RW_RW_OP(operator|,|);

#define MAKE_RW_UNI_OP(OPNM,OP)                 \
    reg_word OPNM() const {                     \
    reg_word newWord;                           \
    newWord.data = OP data;                     \
    newWord.origData = OP origData;             \
    /* needed? */                               \
    newWord.checkMathCorrect();                 \
    return newWord;                             \
    }
    
    MAKE_RW_UNI_OP(operator-,-);
    MAKE_RW_UNI_OP(operator~,~);

    // memory faults

    static bool checkDataOK(const reg_word &val, size_t sz) {
        // mask data to see if it is actually correct
        uint32_t maskedCorrectVal = val.origData;
        if (sz == 2) {
            maskedCorrectVal &= 0xffff;
        } else if (sz == 1) {
            maskedCorrectVal &= 0xff;
        }

        uint32_t maskedVal = val.data;
        if (sz == 2) {
            maskedVal &= 0xffff;
        } else if (sz == 1) {
            maskedVal &= 0xff;
        }

        return maskedVal == maskedCorrectVal;        
    }

    static void checkRecordWriteFaults(reg_word &addr, 
                                       reg_word &val, 
                                       size_t sz) {
        using namespace faultTrack;

        bool addrOK = (addr.data == addr.origData);
        bool valOK = checkDataOK(val, sz);

        // check & record addr
        if (!addrOK) {
            addr.faults.push_back(faultDesc(now, WRITE_ADDR_ERROR));
            // record two errors, because we (probably) mess up the
            // place we write and the place we _fail_ to write
            faultStats[faultTrack::WRITE_ADDR_ERROR] += 2;

            faultList_t origFaults = addr.findFaults();            

            // record that the place we wrote to is (probably) wrong.
            memFaults[addr.data] = memFaultDesc(WRONG_ADDR_WRITTEN, origFaults);

            // record that the place we were _supposed_ to write to is
            // (probably) wrong. We know what the value _should_ be
            memFaults[addr.origData] = memFaultDesc(WRONG_ADDR_NOT_WRITTEN, 
                                                    origFaults);
        }

        if (addrOK && !valOK) { // addr is OK, value is wrong
            memFaults[addr.data] = memFaultDesc(RIGHT_ADDR_WRONG_DATA,
                                                    val.findFaults());
        }

        if (addrOK && valOK) {  // both are correct
            memFaults.erase(addr.data);
        }

        //update shadow file
        origMemWrite(addr.origData, val.origData, sz);
    }

    void checkReadForFaults(const reg_word &addr, const int32_t &in, 
                            size_t sz) {
        // NOTE: probably doens't handle unaligned correctly

        // add the (possibly wrong) data in
        data = in;
        
        if (addr.data == addr.origData) {
            // we are pulling from the correct address
            auto i = memFaults.find(addr.origData);
            if (i != memFaults.end()) {
                // we are accessing faulty data. record this
                // should check for unaligned errors?
                origData = origMemRead(addr.origData, sz);
                faults.insert(faults.end(),
                              i->second.origFaults.begin(),
                              i->second.origFaults.end());
                bool corrected = checkMemCorrect();
                if (corrected) {
                    memFaults.erase(i);
                }
            } else {
                // the address and data are good, no fault.
                origData = in;
                assertOrig(in, addr.data, sz); // make sure they match
            }
        } else {
            // we are pulling from the wrong address
            origData = origMemRead(addr.origData, sz);  // get correct data
            // faults should copy from addr and from the memory address
            faults.insert(faults.end(),
                          addr.faults.begin(),
                          addr.faults.end());
            bool corrected = checkMemCorrect();
            if (corrected) {
                memFaults.erase(addr.data);
            }
        }
    }


    // Fault injection
    void addFault(faultDesc f) {
        //data ^= (1 << f.bit); // flip the bit
        data ^= f.bits; // flip the bits

        f.when = now;
        faults.push_back(f);
        faultStats[f.status]++;
    }

    // overwrite. Used for WB_ADDR faults - the data (may) be correct,
    // but it was written to the wrong place.
    void addOverwriteFault(faultDesc f, const reg_word &val) {
        data = val.getData();
       
        f.when = now;
        faults.push_back(f);
        if (data != origData) {
            faultStats[f.status]++;
        }
    }

    // note that this value was NOT written when it should have been
    // (i.e. WB_ADDR error)
    void noteNotWritten(faultDesc f, const reg_word &shouldBe) {
        origData = shouldBe.origData;

        if (data != origData) {
            f.when = now;
            faults.push_back(f);
            faultStats[faultTrack::WB_ADDR_ERROR]++; // record differently?
        } else {
            faultStats[faultTrack::WB_ADDR_NO_ERROR]++; // record differently?
            printf(" Noting Written, but data is correct (0x%x)\n", origData);
        }
    }
    
    void correct_tmr() {
        using namespace faultTrack;
        if (!faults.empty()) {
            for (auto &&i : faults) {
                if(i.status == FAULTED) {
                    i.status= CORRECTED_TMR;
                    faultStats[CORRECTED_TMR]++;
                    i.whenCorrected = now;
                }
            }
        }
        data = origData;
    }
};

#endif





/* Argument passing registers */

#define REG_V0 2
#define REG_A0 4
#define REG_A1 5
#define REG_A2 6
#define REG_A3 7
#define REG_FA0 12
#define REG_SP 29


/* Result registers */

#define REG_RES 2
#define REG_FRES 0


/* $gp registers */

#define REG_GP 28




#define FPR_S(REGNO) (((REGNO) & 0x1) \
		      ? (run_error ("Bit 0 in FP reg spec\n") ? 0.0 : 0.0)\
		      : FGR[REGNO])

#define FPR_D(REGNO) (double) (((REGNO) & 0x1) \
			       ? (run_error ("Bit 0 in FP reg spec\n") ? 0.0 : 0.0)\
			       : FPR[(REGNO) >> 1])

#define FPR_W(REGNO) (FWR[REGNO])


#define SET_FPR_S(REGNO, VALUE) {if ((REGNO) & 0x1) \
				 run_error ("Bit 0 in FP reg spec\n");\
				 else FGR[REGNO] = (double) (VALUE);}

#define SET_FPR_D(REGNO, VALUE) {if ((REGNO) & 0x1) \
				 run_error ("Bit 0 in FP reg spec\n");\
				 else FPR[(REGNO) >> 1] = (double) (VALUE);}

#define SET_FPR_W(REGNO, VALUE) {FWR[REGNO] = (int) (VALUE);}


/* Floating point control and condition registers: */

#define FCR		CPR[1]
#define FPId		(CPR[1][0])
#define FpCond		(CPR[1][31])


#define EntryHI         (CPR[0][0])
#define EntryLO         (CPR[0][1])
#define Index           (CPR[0][2])
#define Random          (CPR[0][3])
#define Context		(CPR[0][4])
#define BadVAddr	(CPR[0][8])
#define Status_Reg	(CPR[0][12])
#define Cause		(CPR[0][13])
#define EPC		(CPR[0][14])
#define PRId		(CPR[0][15])


#define USER_MODE (Status_Reg & 0x2)
#define INTERRUPTS_ON (Status_Reg & 0x1)

#endif //_REG_H
