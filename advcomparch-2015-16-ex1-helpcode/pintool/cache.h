#ifndef CACHE_H
#define CACHE_H

#include <algorithm>
#include <iostream>  // std::cout ...
#include <sstream>   // ostringstream type
#include <cstdlib>   // rand()
#include <time.h>    // for random seed

/*****************************************************************************/
/* Policy about L2 inclusion of L1's content                                 */
/*****************************************************************************/
#ifndef L2_INCLUSIVE
#  define L2_INCLUSIVE 1
#endif
/*****************************************************************************/


/*****************************************************************************/
/* Cache allocation strategy on stores                                       */
/*****************************************************************************/
enum {
    STORE_ALLOCATE = 0,
    STORE_NO_ALLOCATE
};
#ifndef STORE_ALLOCATION
#  define STORE_ALLOCATION STORE_ALLOCATE
#endif
/*****************************************************************************/

#define KILO 1024
#define MEGA (KILO*KILO)
#define GIGA (KILO*MEGA)

typedef UINT64 CACHE_STATS; // type of cache hit/miss counters

/**
 * decimal - string conversion
 **/
static string dec2str(UINT64 v, UINT32 w)
{
    ostringstream o;
    o.width(w);
    o << v;
    string str(o.str());
    return str;
}

/**
 * determines if n is power of 2
 **/
static inline bool IsPowerOf2(UINT32 n)
{
    return ((n & (n - 1)) == 0);
}

/**
 *  Computes floor(log2(n))
 *  Works by finding position of MSB set.
 *  Returns -1 if n == 0.
 **/
static inline INT32 FloorLog2(UINT32 n)
{
    INT32 p = 0;

    if (n == 0) return -1;

    if (n & 0xffff0000) { p += 16; n >>= 16; }
    if (n & 0x0000ff00) { p +=  8; n >>=  8; }
    if (n & 0x000000f0) { p +=  4; n >>=  4; }
    if (n & 0x0000000c) { p +=  2; n >>=  2; }
    if (n & 0x00000002) { p +=  1; }

    return p;
}

/**
 * `CACHE_TAG` class represents an address tag stored in a cache.
 * `INVALID_TAG` is used as an error on functions with CACHE_TAG return type.
 **/
class CACHE_TAG
{
    private:
    ADDRINT _tag;

    public:
    CACHE_TAG(ADDRINT tag = 0) { _tag = tag; }
    bool operator==(const CACHE_TAG &right) const { return _tag == right._tag; }
    operator ADDRINT() const { return _tag; }
};
CACHE_TAG INVALID_TAG(-1);

/**
 * Everything related to cache sets
 **/
namespace CACHE_SET
{

    class LRU
    {
        protected:
        std::vector<CACHE_TAG> _tags;
        UINT32 _associativity;

        public:
        LRU(UINT32 associativity = 8)
        {
            _associativity = associativity;
            _tags.clear();
        }

        VOID SetAssociativity(UINT32 associativity)
        {
            _associativity = associativity;
            _tags.clear();
        }
        UINT32 GetAssociativity() { return _associativity; }

        string Name() { return "LRU"; }

        UINT32 Find(CACHE_TAG tag)
        {
            for (std::vector<CACHE_TAG>::iterator it = _tags.begin();
                 it != _tags.end(); ++it)
            {
                if (*it == tag) { // Tag found, lets make it MRU
                    _tags.erase(it);
                    _tags.push_back(tag);
                    return true;
                }
            }

            return false;
        }

        CACHE_TAG Replace(CACHE_TAG tag)
        {
            CACHE_TAG ret = INVALID_TAG;
            _tags.push_back(tag);
            if (_tags.size() > _associativity) {
                ret = *_tags.begin();
                _tags.erase(_tags.begin());
            }
            return ret;
        }

        VOID DeleteIfPresent(CACHE_TAG tag)
        {
            for (std::vector<CACHE_TAG>::iterator it = _tags.begin();
                 it != _tags.end(); ++it)
            {
                if (*it == tag) { // Tag found, lets make it MRU
                    _tags.erase(it);
                    break;
                }
            }
        }
    };

    class RANDOM
    {
        protected:
        std::vector<CACHE_TAG> _tags;
        UINT32 _associativity;

        public:
        RANDOM(UINT32 associativity = 8)
        {
            _associativity = associativity;
            _tags.clear();
        }

        VOID SetAssociativity(UINT32 associativity)
        {
            _associativity = associativity;
            _tags.clear();
        }
        UINT32 GetAssociativity() { return _associativity; }

        string Name() { return "RANDOM"; }

        UINT32 Find(CACHE_TAG tag)
        {
            for (std::vector<CACHE_TAG>::iterator it = _tags.begin();
                 it != _tags.end(); ++it)
            {
                if (*it == tag) { // Tag found, lets make it MRU
                    return true;
                }
            }

            return false;
        }

        CACHE_TAG Replace(CACHE_TAG tag)
        {
            CACHE_TAG ret = INVALID_TAG;
            _tags.push_back(tag);
            if (_tags.size() > _associativity) {
                /* initialize random seed: */
                srand(time(NULL));
                UINT32 randomIndex = rand() % _tags.size();
                ret = _tags[randomIndex];
                _tags.erase(_tags.begin() + randomIndex);
            }
            return ret;
        }

        VOID DeleteIfPresent(CACHE_TAG tag)
        {
            for (std::vector<CACHE_TAG>::iterator it = _tags.begin();
                 it != _tags.end(); ++it)
            {
                if (*it == tag) { // Tag found, lets make it MRU
                    _tags.erase(it);
                    break;
                }
            }
        }
    };

    class LFU
    {
        protected:
        std::vector<CACHE_TAG> _tags;
        std::vector<UINT32> _frequencies;
        UINT32 _associativity;

        public:
        LFU(UINT32 associativity = 8)
        {
            _associativity = associativity;
            _tags.clear();
            _frequencies.clear();
        }

        VOID SetAssociativity(UINT32 associativity)
        {
            _associativity = associativity;
            _tags.clear();
            _frequencies.clear();
        }
        UINT32 GetAssociativity() { return _associativity; }

        string Name() { return "LFU"; }

        UINT32 Find(CACHE_TAG tag)
        {
            for (std::vector<CACHE_TAG>::iterator it = _tags.begin();
                 it != _tags.end(); ++it)
            {
                if (*it == tag) {
                    UINT32 index = it - _tags.begin();
                    _frequencies[index]++;
                    return true;
                }
            }
            return false;
        }

        CACHE_TAG Replace(CACHE_TAG tag) {
            CACHE_TAG ret = INVALID_TAG;
            _tags.push_back(tag);
            _frequencies.push_back(1);

            if (_tags.size() <= _associativity) {
                return ret;
            }

            std::vector<UINT32>::iterator minFrequency = std::min_element(_frequencies.begin(), _frequencies.end());
            UINT32 index = minFrequency - _frequencies.begin();
            ret = _tags[index];
            _tags.erase(_tags.begin() + index);
            _frequencies.erase(_frequencies.begin() + index);
            return ret;
        }

        VOID DeleteIfPresent(CACHE_TAG tag) {
            for (std::vector<CACHE_TAG>::iterator it = _tags.begin();
                 it != _tags.end(); ++it)
            {
                if (*it == tag) { // Tag found, lets make it MRU
                    UINT32 index = it - _tags.begin();
                    _tags.erase(it);
                    _frequencies.erase(_frequencies.begin() + index);
                    break;
                }
            }
        }
    };

} // namespace CACHE_SET

template <class SET>
    class TWO_LEVEL_CACHE
{
    public:
    typedef enum
    {
        ACCESS_TYPE_LOAD,
        ACCESS_TYPE_STORE,
        ACCESS_TYPE_NUM
    } ACCESS_TYPE;

    private:
    enum {
        HIT_L1 = 0,
        HIT_L2,
        MISS_L2,
        ACCESS_RESULT_NUM
    };

    static const UINT32 HIT_MISS_NUM = 2;
    CACHE_STATS _l1_access[ACCESS_TYPE_NUM][HIT_MISS_NUM];
    CACHE_STATS _l2_access[ACCESS_TYPE_NUM][HIT_MISS_NUM];

    UINT32 _latencies[ACCESS_RESULT_NUM];

    SET *_l1_sets;
    SET *_l2_sets;

    const std::string _name;
    const UINT32 _l1_cacheSize;
    const UINT32 _l2_cacheSize;
    const UINT32 _l1_blockSize;
    const UINT32 _l2_blockSize;
    const UINT32 _l1_associativity;
    const UINT32 _l2_associativity;

    // computed params
    const UINT32 _l1_lineShift; // i.e. no of block offset bits
    const UINT32 _l2_lineShift;
    const UINT32 _l1_setIndexMask; // mask applied to get the set index
    const UINT32 _l2_setIndexMask;

    CACHE_STATS L1SumAccess(bool hit) const
    {
        CACHE_STATS sum = 0;
        for (UINT32 accessType = 0; accessType < ACCESS_TYPE_NUM; accessType++)
            sum += _l1_access[accessType][hit];
        return sum;
    }
    CACHE_STATS L2SumAccess(bool hit) const
    {
        CACHE_STATS sum = 0;
        for (UINT32 accessType = 0; accessType < ACCESS_TYPE_NUM; accessType++)
            sum += _l2_access[accessType][hit];
        return sum;
    }

    UINT32 L1NumSets() const { return _l1_setIndexMask + 1; }
    UINT32 L2NumSets() const { return _l2_setIndexMask + 1; }

    // accessors
    UINT32 L1CacheSize() const { return _l1_cacheSize; }
    UINT32 L2CacheSize() const { return _l2_cacheSize; }
    UINT32 L1BlockSize() const { return _l1_blockSize; }
    UINT32 L2BlockSize() const { return _l2_blockSize; }
    UINT32 L1Associativity() const { return _l1_associativity; }
    UINT32 L2Associativity() const { return _l2_associativity; }
    UINT32 L1LineShift() const { return _l1_lineShift; }
    UINT32 L2LineShift() const { return _l2_lineShift; }
    UINT32 L1SetIndexMask() const { return _l1_setIndexMask; }
    UINT32 L2SetIndexMask() const { return _l2_setIndexMask; }

    VOID SplitAddress(const ADDRINT addr, UINT32 lineShift, UINT32 setIndexMask,
                      CACHE_TAG & tag, UINT32 & setIndex) const
    {
        tag = addr >> lineShift;
        setIndex = tag & setIndexMask;
        tag = tag >> FloorLog2(L1NumSets());
    }


    public:
    // constructors/destructors
    TWO_LEVEL_CACHE(std::string name,
                    UINT32 l1CacheSize, UINT32 l1BlockSize, UINT32 l1Associativity,
                    UINT32 l2CacheSize, UINT32 l2BlockSize, UINT32 l2Associativity,
                    UINT32 l1HitLatency = 1, UINT32 l2HitLatency = 10,
                    UINT32 l2MissLatency = 150);

    // Stats
    CACHE_STATS L1Hits(ACCESS_TYPE accessType) const { return _l1_access[accessType][true];}
    CACHE_STATS L2Hits(ACCESS_TYPE accessType) const { return _l2_access[accessType][true];}
    CACHE_STATS L1Misses(ACCESS_TYPE accessType) const { return _l1_access[accessType][false];}
    CACHE_STATS L2Misses(ACCESS_TYPE accessType) const { return _l2_access[accessType][false];}
    CACHE_STATS L1Accesses(ACCESS_TYPE accessType) const { return L1Hits(accessType) + L1Misses(accessType);}
    CACHE_STATS L2Accesses(ACCESS_TYPE accessType) const { return L2Hits(accessType) + L2Misses(accessType);}
    CACHE_STATS L1Hits() const { return L1SumAccess(true);}
    CACHE_STATS L2Hits() const { return L2SumAccess(true);}
    CACHE_STATS L1Misses() const { return L1SumAccess(false);}
    CACHE_STATS L2Misses() const { return L2SumAccess(false);}
    CACHE_STATS L1Accesses() const { return L1Hits() + L1Misses();}
    CACHE_STATS L2Accesses() const { return L2Hits() + L2Misses();}

    string StatsLong(string prefix = "") const;
    string PrintCache(string prefix = "") const;

    UINT32 Access(ADDRINT addr, ACCESS_TYPE accessType);
};

template <class SET>
    TWO_LEVEL_CACHE<SET>::TWO_LEVEL_CACHE(
        std::string name,
        UINT32 l1CacheSize, UINT32 l1BlockSize, UINT32 l1Associativity,
        UINT32 l2CacheSize, UINT32 l2BlockSize, UINT32 l2Associativity,
        UINT32 l1HitLatency, UINT32 l2HitLatency, UINT32 l2MissLatency)
        : _name(name),
_l1_cacheSize(l1CacheSize),
_l2_cacheSize(l2CacheSize),
_l1_blockSize(l1BlockSize),
_l2_blockSize(l2BlockSize),
_l1_associativity(l1Associativity),
_l2_associativity(l2Associativity),
_l1_lineShift(FloorLog2(l1BlockSize)),
_l2_lineShift(FloorLog2(l2BlockSize)),
_l1_setIndexMask((l1CacheSize / (l1Associativity * l1BlockSize)) - 1),
_l2_setIndexMask((l2CacheSize / (l2Associativity * l2BlockSize)) - 1)
{

    // They all need to be power of 2
    ASSERTX(IsPowerOf2(_l1_blockSize));
    ASSERTX(IsPowerOf2(_l2_blockSize));
    ASSERTX(IsPowerOf2(_l1_setIndexMask + 1));
    ASSERTX(IsPowerOf2(_l2_setIndexMask + 1));

    // Some more sanity checks
    ASSERTX(_l1_cacheSize <= _l2_cacheSize);
    ASSERTX(_l1_blockSize <= _l2_blockSize);

    // Allocate space for L1 and L2 sets
    _l1_sets = new SET[L1NumSets()];
    _l2_sets = new SET[L2NumSets()];

    _latencies[HIT_L1] = l1HitLatency;
    _latencies[HIT_L2] = l2HitLatency;
    _latencies[MISS_L2] = l2MissLatency;

    for (UINT32 i = 0; i < L1NumSets(); i++)
        _l1_sets[i].SetAssociativity(_l1_associativity);
    for (UINT32 i = 0; i < L2NumSets(); i++)
        _l2_sets[i].SetAssociativity(_l2_associativity);

    for (UINT32 accessType = 0; accessType < ACCESS_TYPE_NUM; accessType++)
    {
        _l1_access[accessType][false] = 0;
        _l2_access[accessType][true] = 0;
        _l1_access[accessType][false] = 0;
        _l2_access[accessType][true] = 0;
    }
}

template <class SET>
    string TWO_LEVEL_CACHE<SET>::StatsLong(string prefix) const
    {
        const UINT32 headerWidth = 19;
        const UINT32 numberWidth = 12;

        string out;

        // L1 stats first
        out += prefix + "L1 Cache Stats:" + "\n";

        for (UINT32 i = 0; i < ACCESS_TYPE_NUM; i++)
        {
            const ACCESS_TYPE accessType = ACCESS_TYPE(i);

            std::string type(accessType == ACCESS_TYPE_LOAD ? "L1-Load" : "L1-Store");

            out += prefix + ljstr(type + "-Hits:      ", headerWidth)
                + dec2str(L1Hits(accessType), numberWidth)  +
                "  " +fltstr(100.0 * L1Hits(accessType) / L1Accesses(accessType), 2, 6) + "%\n";

            out += prefix + ljstr(type + "-Misses:    ", headerWidth)
                + dec2str(L1Misses(accessType), numberWidth) +
                "  " +fltstr(100.0 * L1Misses(accessType) / L1Accesses(accessType), 2, 6) + "%\n";

            out += prefix + ljstr(type + "-Accesses:  ", headerWidth)
                + dec2str(L1Accesses(accessType), numberWidth) +
                "  " +fltstr(100.0 * L1Accesses(accessType) / L1Accesses(accessType), 2, 6) + "%\n";

            out += prefix + "\n";
        }

        out += prefix + ljstr("L1-Total-Hits:      ", headerWidth)
            + dec2str(L1Hits(), numberWidth) +
            "  " +fltstr(100.0 * L1Hits() / L1Accesses(), 2, 6) + "%\n";

        out += prefix + ljstr("L1-Total-Misses:    ", headerWidth)
            + dec2str(L1Misses(), numberWidth) +
            "  " +fltstr(100.0 * L1Misses() / L1Accesses(), 2, 6) + "%\n";

        out += prefix + ljstr("L1-Total-Accesses:  ", headerWidth)
            + dec2str(L1Accesses(), numberWidth) +
            "  " +fltstr(100.0 * L1Accesses() / L1Accesses(), 2, 6) + "%\n";
        out += "\n";


        // L2 Stats now.
        out += prefix + "L2 Cache Stats:" + "\n";

        for (UINT32 i = 0; i < ACCESS_TYPE_NUM; i++)
        {
            const ACCESS_TYPE accessType = ACCESS_TYPE(i);

            std::string type(accessType == ACCESS_TYPE_LOAD ? "L2-Load" : "L2-Store");

            out += prefix + ljstr(type + "-Hits:      ", headerWidth)
                + dec2str(L2Hits(accessType), numberWidth)  +
                "  " +fltstr(100.0 * L2Hits(accessType) / L2Accesses(accessType), 2, 6) + "%\n";

            out += prefix + ljstr(type + "-Misses:    ", headerWidth)
                + dec2str(L2Misses(accessType), numberWidth) +
                "  " +fltstr(100.0 * L2Misses(accessType) / L2Accesses(accessType), 2, 6) + "%\n";

            out += prefix + ljstr(type + "-Accesses:  ", headerWidth)
                + dec2str(L2Accesses(accessType), numberWidth) +
                "  " +fltstr(100.0 * L2Accesses(accessType) / L2Accesses(accessType), 2, 6) + "%\n";

            out += prefix + "\n";
        }

        out += prefix + ljstr("L2-Total-Hits:      ", headerWidth)
            + dec2str(L2Hits(), numberWidth) +
            "  " +fltstr(100.0 * L2Hits() / L2Accesses(), 2, 6) + "%\n";

        out += prefix + ljstr("L2-Total-Misses:    ", headerWidth)
            + dec2str(L2Misses(), numberWidth) +
            "  " +fltstr(100.0 * L2Misses() / L2Accesses(), 2, 6) + "%\n";

        out += prefix + ljstr("L2-Total-Accesses:  ", headerWidth)
            + dec2str(L2Accesses(), numberWidth) +
            "  " +fltstr(100.0 * L2Accesses() / L2Accesses(), 2, 6) + "%\n";
        out += prefix + "\n";

        return out;
    }

template <class SET>
    string TWO_LEVEL_CACHE<SET>::PrintCache(string prefix) const
    {
        string out;

        out += prefix + _name + ":\n";
        out += prefix + "  L1-Data Cache:\n";
        out += prefix + "    Size(KB):       " + dec2str(this->L1CacheSize()/KILO, 5) + "\n";
        out += prefix + "    Block Size(B):  " + dec2str(this->L1BlockSize(), 5) + "\n";
        out += prefix + "    Associativity:  " + dec2str(this->L1Associativity(), 5) + "\n";
        out += prefix + "\n";
        out += prefix + "  L2-Data Cache:\n";
        out += prefix + "    Size(KB):       " + dec2str(this->L2CacheSize()/KILO, 5) + "\n";
        out += prefix + "    Block Size(B):  " + dec2str(this->L2BlockSize(), 5) + "\n";
        out += prefix + "    Associativity:  " + dec2str(this->L2Associativity(), 5) + "\n";
        out += prefix + "\n";

        out += prefix + "Latencies: " + dec2str(_latencies[HIT_L1], 4) + " "
            + dec2str(_latencies[HIT_L2], 4) + " "
            + dec2str(_latencies[MISS_L2], 4) + "\n";
        out += prefix + "L1-Sets: " + this->_l1_sets[0].Name() + " assoc: " +
            dec2str(this->_l1_sets[0].GetAssociativity(), 3) + "\n";
        out += prefix + "L2-Sets: " + this->_l2_sets[0].Name() + " assoc: " +
            dec2str(this->_l2_sets[0].GetAssociativity(), 3) + "\n";
        out += prefix + "Store_allocation: " + (STORE_ALLOCATION == STORE_ALLOCATE ? "Yes" : "No") + "\n";
        out += prefix + "L2_inclusive: " + (L2_INCLUSIVE == 1 ? "Yes" : "No") + "\n";
        out += "\n";

        return out;
    }

// Returns the cycles to serve the request.
template <class SET>
    UINT32 TWO_LEVEL_CACHE<SET>::Access(ADDRINT addr, ACCESS_TYPE accessType)
    {
        CACHE_TAG l1Tag, l2Tag;
        UINT32 l1SetIndex, l2SetIndex;
        bool l1Hit = 0, l2Hit = 0;
        UINT32 cycles = 0;

        // Let's check L1 first
        SplitAddress(addr, L1LineShift(), L1SetIndexMask(), l1Tag, l1SetIndex);
        SET & l1Set = _l1_sets[l1SetIndex];
        l1Hit = l1Set.Find(l1Tag);
        _l1_access[accessType][l1Hit]++;
        cycles = _latencies[HIT_L1];

        if (!l1Hit) {
            // On miss, loads always allocate, stores optionally
            if (accessType == ACCESS_TYPE_LOAD ||
                STORE_ALLOCATION == STORE_ALLOCATE)
                l1Set.Replace(l1Tag);

            // Let's check L2 now
            SplitAddress(addr, L2LineShift(), L2SetIndexMask(), l2Tag, l2SetIndex);
            SET & l2Set = _l2_sets[l2SetIndex];
            l2Hit = l2Set.Find(l2Tag);
            _l2_access[accessType][l2Hit]++;
            cycles += _latencies[HIT_L2];

            // L2 always allocates loads and stores
            if (!l2Hit) {
                CACHE_TAG l2_replaced = l2Set.Replace(l2Tag);
                cycles += _latencies[MISS_L2];

                // If L2 is inclusive and a TAG has been replaced we need to remove
                // all evicted blocks from L1.
                if ((L2_INCLUSIVE == 1) && !(l2_replaced == INVALID_TAG)) {
                    ADDRINT replacedAddr = ADDRINT(l2Tag) << FloorLog2(L2NumSets());
                    replacedAddr = replacedAddr | l2SetIndex;
                    replacedAddr = replacedAddr << L2LineShift();
                    for (UINT32 i=0; i < L2BlockSize(); i+=L1BlockSize()) {
                        ADDRINT newAddr = replacedAddr | i;
                        SplitAddress(newAddr, L1LineShift(), L1SetIndexMask(), l1Tag, l1SetIndex);
                        l1Set = _l1_sets[l1SetIndex];
                        l1Set.DeleteIfPresent(l1Tag);
                    }
                }
            }
        }

        return cycles;
    }

#endif // CACHE_H

