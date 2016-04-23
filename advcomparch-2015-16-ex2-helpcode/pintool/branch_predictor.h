#ifndef BRANCH_PREDICTOR_H
#define BRANCH_PREDICTOR_H

#include <sstream> // std::ostringstream
#include <cmath>   // pow()
#include <cstring> // memset()

/**
 * A generic BranchPredictor base class.
 * All predictors can be subclasses with overloaded predict() and update()
 * methods.
 **/
class BranchPredictor
{
    public:
        BranchPredictor() : correct_predictions(0), incorrect_predictions(0) {};
        ~BranchPredictor() {};

        virtual bool predict(ADDRINT ip, ADDRINT target) = 0;
        virtual void update(bool predicted, bool actual, ADDRINT ip, ADDRINT target) = 0;
        virtual string getName() = 0;

        UINT64 getNumCorrectPredictions() { return correct_predictions; }
        UINT64 getNumIncorrectPredictions() { return incorrect_predictions; }

        void resetCounters() { correct_predictions = incorrect_predictions = 0; };

    protected:
        void updateCounters(bool predicted, bool actual) {
            if (predicted == actual)
                correct_predictions++;
            else
                incorrect_predictions++;
        };

    private:
        UINT64 correct_predictions;
        UINT64 incorrect_predictions;
};

class NbitPredictor : public BranchPredictor
{
    public:
        NbitPredictor(unsigned index_bits_, unsigned cntr_bits_)
            : BranchPredictor(), index_bits(index_bits_), cntr_bits(cntr_bits_) {
                table_entries = 1 << index_bits;
                TABLE = new unsigned long long[table_entries];
                memset(TABLE, 0, table_entries * sizeof(*TABLE));

                COUNTER_MAX = (1 << cntr_bits) - 1;
            };
        ~NbitPredictor() { delete TABLE; };

        virtual bool predict(ADDRINT ip, ADDRINT target) {
            unsigned int ip_table_index = ip % table_entries;
            unsigned long long ip_table_value = TABLE[ip_table_index];
            unsigned long long prediction = ip_table_value >> (cntr_bits - 1);
            return (prediction != 0);
        };

        virtual void update(bool predicted, bool actual, ADDRINT ip, ADDRINT target) {
            unsigned int ip_table_index = ip % table_entries;
            if (actual) {
                if (TABLE[ip_table_index] < COUNTER_MAX)
                    TABLE[ip_table_index]++;
            } else {
                if (TABLE[ip_table_index] > 0)
                    TABLE[ip_table_index]--;
            }

            updateCounters(predicted, actual);
        };

        virtual string getName() {
            std::ostringstream stream;
            stream << "Nbit-" << pow(2,index_bits) / 1024.0 << "K-" << cntr_bits;
            return stream.str();
        }

        unsigned long long GetVal(ADDRINT ip, ADDRINT target) {
            unsigned int ip_table_index = ip % table_entries;
            unsigned long long ip_table_value = TABLE[ip_table_index];
            return ip_table_value;
        }

    private:
        unsigned int index_bits, cntr_bits;
        unsigned int COUNTER_MAX;

        /* Make this unsigned long long so as to support big numbers of cntr_bits. */
        unsigned long long *TABLE;
        unsigned int table_entries;
};

// Fill in the BTB implementation ...
// so, it seems like the way our code should work is this:
// our methods get called only when a branch instruction
// is encountered. We run predict(), essentialy looking for the
// ip we were provided with. If we find it, then we return true,
// else false
// Immediately afterwards, we run update(). If prediction is false, but
// actual is true, then we need to add this branch instruction to our btb.
// If both are false, nothing to do. If prediction is true, we check actual
// if actual is true, then we check the addresses? if they match, we made a
// correct prediction. however, if they don't, then we made a wrong call? so what
// should we do with our entry there? If actual is false, then we need to erase
// the previous entry from the btb...
class BTBPredictor : public BranchPredictor
{
    public:
        BTBPredictor(int btb_lines, int btb_assoc)
            : table_lines(btb_lines), table_assoc(btb_assoc)
        {
            entries = new ADDRINT[table_lines * table_assoc];
            addresses = new ADDRINT[table_lines * table_assoc];
            frequencies = new unsigned long[table_lines * table_assoc];

            correctTargetPredictions = 0;
            wrongTargetPredictions = 0;

            int rem = table_lines;
            pcMask = 0;
            /*calculate how many bits to use from the PC*/
            //this could be faster
            while (rem >>= 1) {
                pcMask <<= 1;
                pcMask |= 1;
            }

            memset(entries, 0, table_lines * table_assoc * sizeof(*entries));
            memset(addresses, 0, table_lines * table_assoc * sizeof(*addresses));
            memset(frequencies, 0, table_lines * table_assoc * sizeof(*frequencies));
        }

        ~BTBPredictor() {
            delete entries;
            delete addresses;
            delete frequencies;
        }

        virtual bool predict(ADDRINT ip, ADDRINT target) {
            int index = ip & pcMask; //from here, stride to next of set is table_lines / table_assoc
            int step = table_lines / table_assoc;

            while (index < table_lines) {
                if (entries[index] == ip) {
                    frequencies[index]++;
                    return true;
                }

                index += step;
            }

            return false;
        }

        virtual void update(bool predicted, bool actual, ADDRINT ip, ADDRINT target) {
            if (predicted) {
                int index = FindInTable(ip);
                if (actual) {
                    //do we _really_ need this check
                    if (addresses[index] != target) {
                        //we made a wrong address prediction. what do we do now?
                        //change the value of addresses[index], or remove that entry from
                        //the table and then move on?
                        addresses[index] = target;
                        wrongTargetPredictions++;
                    }else {
                        correctTargetPredictions++;
                    }
                }else {
                    //wrong prediction
                    entries[index] = 0;
                    addresses[index] = 0;
                    frequencies[index] = 0;
                }
            }else {
                if (actual) {
                    int index = FindInTable(ip);
                    entries[index] = ip;
                    addresses[index] = target;
                    frequencies[index] = 0;
                }else {
                    //should this count as a correct prediction?
                }
            }
            updateCounters(predicted, actual);
        }

        virtual string getName() { 
            std::ostringstream stream;
            stream << "BTB-" << table_lines << "-" << table_assoc;
            return stream.str();
        }


        UINT64 getNumInorrectTargetPredictions() { 
            return wrongTargetPredictions;
        }

        UINT64 getNumCorrectTargetPredictions() { 
            return correctTargetPredictions;
        }

    private:
        int table_lines, table_assoc;
        int pcMask;

        UINT64 correctTargetPredictions;
        UINT64 wrongTargetPredictions;

        ADDRINT *entries;
        ADDRINT *addresses;
        unsigned long *frequencies;

        int FindInTable(ADDRINT ip) {
            //if the entry we are looking for doesn't exist, this returns the index of the
            //element we should replace
            int index = ip & pcMask, minIndex = index;
            int step = table_lines / table_assoc;
            unsigned long minFreq = 0;
            minFreq--;

            while (index < table_lines) {
                if (entries[index] == ip) {
                    return index;
                }else {
                    if (minFreq > frequencies[index]) {
                        minFreq = frequencies[index];
                        minIndex = index;
                    }
                }

                index += step;
            }

            return minIndex;
        }

};

class static_not_taken_predictor : public BranchPredictor {
    public:
        virtual bool predict(ADDRINT ip, ADDRINT target) {
            return false;
        }

        virtual void update(bool predicted, bool actual, ADDRINT ip, ADDRINT target) {
            updateCounters(predicted, actual);
        }

        virtual string getName() { 
            std::ostringstream stream;
            stream << "StaticNotTaken";
            return stream.str();
        }
};

class btfnt_predictor : public BranchPredictor {
    public:
        virtual bool predict(ADDRINT ip, ADDRINT target) {
            if (ip < target)
                return false;

            return true;
        }

        virtual void update(bool predicted, bool actual, ADDRINT ip, ADDRINT target) {
            updateCounters(predicted, actual);
        }

        virtual string getName() { 
            std::ostringstream stream;
            stream << "BTFNT";
            return stream.str();
        }

};

class local_two_level_predictor : public BranchPredictor {
    public:
        local_two_level_predictor(int pEntries, int pLen, int bEntries, int bLen) : 
            phtEntries(pEntries), phtCounterLength(pLen), bhtEntries(bEntries), bhtEntryLength(bLen) {
                pht = new unsigned int[phtEntries];
                bht = new unsigned int[bhtEntries];

                phtCounterMax = pow(2, phtCounterLength) - 1;

                memset(pht, 0, phtEntries * sizeof(*pht));
                memset(bht, 0, bhtEntries * sizeof(*bht));
            }

        ~local_two_level_predictor() {
            delete pht;
            delete bht;
        }

        virtual bool predict(ADDRINT ip, ADDRINT target) {
            unsigned int bhtIndex = ip % bhtEntries;
            unsigned int phtSuffix = bht[bhtIndex];
            unsigned int temp = ip << bhtEntryLength;
            unsigned int phtIndex = (temp + phtSuffix) % phtEntries;

            //phtIndex <<= bhtEntryLength;
            //phtIndex |= phtSuffix;

            unsigned int phtRes = pht[phtIndex];
            phtRes >>= (phtCounterLength - 1);

            return (phtRes != 0);
        }

        virtual void update(bool predicted, bool actual, ADDRINT ip, ADDRINT target) {
            unsigned int bhtIndex = ip % bhtEntries;
            unsigned int phtSuffix = bht[bhtIndex];
            unsigned int temp = ip << bhtEntryLength;
            unsigned int phtIndex = (temp + phtSuffix) % phtEntries;

            //phtIndex <<= bhtEntryLength;
            //phtIndex |= phtSuffix;

            bht[bhtIndex] <<= 1;

            if (actual) {
                bht[bhtIndex] |= 1;
                if (pht[phtIndex] < phtCounterMax) {
                    pht[phtIndex]++;
                }
            }else {
                if (pht[phtIndex] > 0) {
                    pht[phtIndex]--;
                } 
            }

            updateCounters(predicted, actual);
        }

        virtual string getName() {
            std::ostringstream stream;
            stream << "LocalTwoLevel-" << phtEntries << "-" << bhtEntries << "-" << bhtEntryLength;
            return stream.str();
        }

    private:
        unsigned int* pht;
        unsigned int* bht;

        unsigned int bhtMask;
        unsigned int phtMask;

        unsigned int phtEntries;
        unsigned int phtCounterLength;
        unsigned int phtCounterMax;
        unsigned int bhtEntries;
        unsigned int bhtEntryLength;

        unsigned int pBits;
        unsigned int bBits;
};

class global_two_level_predictor : public BranchPredictor {
    public:
        global_two_level_predictor(int pentries, int plen, int blen) : 
            phtEntries(pentries), phtCounterLength(plen), bhrLength(blen) {
                pht = new unsigned int[phtEntries];
                bhr = 0;
                step = pow(2, bhrLength);

                /*
                   int rem = bhrLength;
                   bhrMask = 0;

                   while (rem >>= 1) {
                   bhrMask<<=1;
                   bhrMask |= 1;
                   }

                   rem = step;
                   phtMask = 0;

                   while (rem >>=1) {
                   phtMask<<=1;
                   phtMask |= 1;
                   }
                   */

                phtCounterMax = pow(2, phtCounterLength) - 1;
                memset(pht, 0, phtEntries * sizeof(*pht));
            }

        ~global_two_level_predictor() {
            delete pht;
        }

        virtual bool predict(ADDRINT ip, ADDRINT target) {
            unsigned int phtIndex = ip % (phtEntries / step);

            unsigned int phtRes = pht[phtIndex + (bhr * step)];
            phtRes >>= (phtCounterLength - 1);

            return (phtRes != 0);
        }

        virtual void update(bool predicted, bool actual, ADDRINT ip, ADDRINT target) {
            unsigned int phtIndex = (ip % (phtEntries / step)) + (bhr * step);

            bhr <<= 1;

            if (actual) {
                bhr |= 1;
                if (pht[phtIndex] < phtCounterMax) {
                    pht[phtIndex]++;
                }
            }else {
                if (pht[phtIndex] > 0) {
                    pht[phtIndex]--;
                } 
            }

            bhr %= step;

            updateCounters(predicted, actual);
        }

        virtual string getName() {
            std::ostringstream stream;
            stream << "GlobalTwoLevel-" << phtEntries << "-" << phtCounterLength << "-" << bhrLength;
            return stream.str();
        }

    private:
        unsigned int* pht;
        unsigned int bhr;

        unsigned int step;
        unsigned int phtMask;
        unsigned int bhrMask;

        unsigned int phtEntries;
        unsigned int phtCounterLength;
        unsigned int phtCounterMax;
        unsigned int bhrLength;
};

enum type_enum {NBITPREDICTOR_TYPE, GLOBALPREDICTOR_TYPE, LOCALPREDICTOR_TYPE, COUNT_TYPE};

struct predictor_args {
    int pEntriesBits;
    int pLen;
    int bEntriesBits;
    int bLen;

    predictor_args() {
        pEntriesBits = 0;
        pLen = 0;
        bEntriesBits = 0;
        bLen = 0;
    }
};

class tournament_predictor : public BranchPredictor {
    public:
        tournament_predictor(type_enum p0Type, type_enum p1Type, predictor_args p0Args, predictor_args p1Args) : p0Type_(p0Type), p1Type_(p1Type){
            switch (p0Type) {
                case NBITPREDICTOR_TYPE:
                    p0 = new NbitPredictor(p0Args.pEntriesBits, p0Args.pLen);
                    std::cout << "initted p0\n";
                    break;
                case GLOBALPREDICTOR_TYPE:
                    p0 = new global_two_level_predictor(pow(2, p0Args.pEntriesBits), p0Args.pLen, p0Args.bLen);
                    break;
                case LOCALPREDICTOR_TYPE:
                    p0 = new local_two_level_predictor(pow(2, p0Args.pEntriesBits), p0Args.pLen, pow(2, p0Args.bEntriesBits), p0Args.bLen);
                    break;
                default:
                    p0 = new NbitPredictor(p0Args.pEntriesBits, p0Args.pLen);
                    break;
            }

            switch (p1Type) {
                case NBITPREDICTOR_TYPE:
                    p1 = new NbitPredictor(p1Args.pEntriesBits, p1Args.pLen);
                    break;
                case GLOBALPREDICTOR_TYPE:
                    p1 = new global_two_level_predictor(pow(2, p1Args.pEntriesBits), p1Args.pLen, p1Args.bLen);
                    break;
                case LOCALPREDICTOR_TYPE:
                    p1 = new local_two_level_predictor(pow(2, p1Args.pEntriesBits), p1Args.pLen, pow(2, p1Args.bEntriesBits), p0Args.bLen);
                    break;
                default:
                    p1 = new NbitPredictor(p1Args.pEntriesBits, p1Args.pLen);
                    break;
            }

            metaPredBits = 9;

            metaPredictor = new NbitPredictor(metaPredBits, 2);
        }

        ~tournament_predictor() {
            switch(p0Type_) {
                case NBITPREDICTOR_TYPE:
                    {
                        NbitPredictor *temp = (NbitPredictor *) p0;
                        temp->~NbitPredictor();
                        break;
                    }
                case GLOBALPREDICTOR_TYPE:
                    {
                        global_two_level_predictor *temp = (global_two_level_predictor *) p0;
                        temp->~global_two_level_predictor();
                        break;
                    }
                case LOCALPREDICTOR_TYPE:
                    {
                        local_two_level_predictor *temp = (local_two_level_predictor *) p0;
                        temp->~local_two_level_predictor();
                        break;
                    }
                default:
                    {
                        NbitPredictor *temp = (NbitPredictor *) p0;
                        temp->~NbitPredictor();
                        break;
                    }
            }

            switch(p1Type_) {
                case NBITPREDICTOR_TYPE:
                    {
                        NbitPredictor *temp = (NbitPredictor *) p1;
                        temp->~NbitPredictor();
                        break;
                    }
                case GLOBALPREDICTOR_TYPE:
                    {
                        global_two_level_predictor *temp = (global_two_level_predictor *) p1;
                        temp->~global_two_level_predictor();
                        break;
                    }
                case LOCALPREDICTOR_TYPE:
                    {
                        local_two_level_predictor *temp = (local_two_level_predictor *) p1;
                        temp->~local_two_level_predictor();
                        break;
                    }
                default:
                    {
                        NbitPredictor *temp = (NbitPredictor *) p1;
                        temp->~NbitPredictor();
                        break;
                    }
            }

            metaPredictor->~NbitPredictor();
        }

        virtual bool predict(ADDRINT ip, ADDRINT target) {
            bool metaPrediction = metaPredictor->predict(ip, target);

            if (metaPrediction) {
                return p1->predict(ip, target);
            }else {
                return p0->predict(ip, target);
            }

            return false;
        }


        virtual void update(bool predicted, bool actual, ADDRINT ip, ADDRINT target) {
            bool metaPrediction = metaPredictor->predict(ip, target);
            bool p0Prediction = p0->predict(ip, target);
            bool p1Prediction = p1->predict(ip, target);

            bool p0Correct = (p0Prediction == actual);
            bool p1Correct = (p1Prediction == actual);

            if (metaPrediction) {
                p1->update(predicted, actual, ip, target);

            }else {
                p0->update(predicted, actual, ip, target);
            }

            if ((p0Correct != p1Correct)) {
                if (p0Correct) {
                    metaPredictor->update(metaPrediction, false, ip, target);
                }else if (p1Correct) {
                    metaPredictor->update(metaPrediction, true, ip, target);
                }
            }

            updateCounters(predicted, actual);
        }

        virtual string getName() {
            std::ostringstream stream;
#if 0
            switch(p0Type_) {
                case NBITPREDICTOR_TYPE:
                    types += "Nbit - ";
                    break;
                case GLOBALPREDICTOR_TYPE:
                    types += "GlobalPredictor - ";
                    break;
                case LOCALPREDICTOR_TYPE:
                    types += "LocalPredictor - ";
                    break;
                default:
                    types += "Nbit - ";
                    break;
            }

            switch(p1Type_) {
                case NBITPREDICTOR_TYPE:
                    types += "Nbit";
                    break;
                case GLOBALPREDICTOR_TYPE:
                    types += "GlobalPredictor";
                    break;
                case LOCALPREDICTOR_TYPE:
                    types += "LocalPredictor";
                    break;
                default:
                    types += "Nbit";
                    break;
            }
#endif

            stream << "Tournament-" << 512 << "-" << p0->getName() << "||" << p1->getName(); 
            return stream.str();
        }

    private:
        BranchPredictor* p0;
        type_enum p0Type_;
        BranchPredictor* p1;
        type_enum p1Type_;
        NbitPredictor* metaPredictor;

        unsigned int metaPredBits;
};
#endif
