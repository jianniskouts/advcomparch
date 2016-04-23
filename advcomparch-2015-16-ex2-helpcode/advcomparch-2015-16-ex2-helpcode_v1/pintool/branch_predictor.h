#ifndef BRANCH_PREDICTOR_H
#define BRANCH_PREDICTOR_H

#include <sstream> // std::ostringstream
#include <cmath>   // pow()
#include <cstring> // memset()
#include <iostream> // memset()

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

private:
    unsigned int index_bits, cntr_bits;
    unsigned int COUNTER_MAX;

    /* Make this unsigned long long so as to support big numbers of cntr_bits. */
    unsigned long long *TABLE;
    unsigned int table_entries;
};

// Fill in the BTB implementation ...
class BTBPredictor : public BranchPredictor
{
public:
	BTBPredictor(int btb_lines, int btb_assoc)
	     :BranchPredictor(), table_lines(btb_lines), table_assoc(btb_assoc)
	{
		TABLE_IP = new ADDRINT*[table_lines];
		TABLE_TARGET = new ADDRINT*[table_lines];
		for(int i = 0;i < table_lines; i++){
        	TABLE_IP[i] = new ADDRINT[table_assoc];
         	memset(TABLE_IP[i], 0, table_assoc * sizeof(**TABLE_IP));
        	TABLE_TARGET[i] = new ADDRINT[table_assoc];
         	memset(TABLE_TARGET[i], 0, table_assoc * sizeof(**TABLE_TARGET));
         }
		 TABLE_INDEX = new unsigned int[table_lines];
         memset(TABLE_INDEX, 0, table_lines * sizeof(*TABLE_INDEX));
	}

	~BTBPredictor() {
        delete TABLE_IP;
        delete TABLE_TARGET;
        delete TABLE_INDEX;
    }

    virtual bool predict(ADDRINT ip, ADDRINT target) {
        unsigned int line_index = ip % table_lines;
        ADDRINT* line = TABLE_IP[line_index];
        for (int i=0; i < table_assoc; ++i) {
			if (ip == line[i]) {
				return true;
            }
        }
		return false;
	}

    virtual void update(bool predicted, bool actual, ADDRINT ip, ADDRINT target) {
 //      updateCounters(predicted, actual);

        // If both false, return
        if (predicted == actual && !actual) {
        	updateCounters(predicted, actual);
			return;
        }

        // If both true, check targets
        if (predicted == actual) {
            unsigned int line_index = ip % table_lines;
            ADDRINT* line = TABLE_IP[line_index];
            for ( int i = 0; i < table_assoc; ++i) {
                if (ip == line[i]) {
                    if (TABLE_TARGET[line_index][i] == target) {
                        correct_targets++;
                 	}else{
						incorrect_targets++;
					}
                }
            }
        	updateCounters(predicted, actual);
        }
        // We decided not to take, but branch got taken. Add in cache.
        if (actual) {
            unsigned int line_index = ip % table_lines;
            unsigned int index = TABLE_INDEX[line_index];
            TABLE_IP[line_index][index] = ip;
            TABLE_TARGET[line_index][index] = target;
            TABLE_INDEX[line_index] = index++ % table_assoc;
        	updateCounters(predicted, actual);
        }

        // We decided to take, but branch did not get taken. Remove from cache.
        if (predicted) {
            unsigned int line_index = ip % table_lines;
            ADDRINT* line = TABLE_IP[line_index];
            for (int i = 0; i < table_assoc; ++i) {
                if (ip == line[i]) {
                    line[i] = 0;
                    TABLE_TARGET[line_index][i] = 0;
                }
            }
        	updateCounters(predicted, actual);
        }
        //updateCounters(predicted, actual);
		getNumCorrectTargetPredictions();
		getNumIncorrectTargetPredictions();
	}

    virtual string getName() {
        std::ostringstream stream;
		stream << "BTB-" << table_lines << "-" << table_assoc;
		return stream.str();
	}

    UINT64 getNumCorrectTargetPredictions() { return correct_targets;}

	UINT64	getNumIncorrectTargetPredictions(){ return incorrect_targets;}

private:
	int lines;
	int table_lines, table_assoc,correct_targets,incorrect_targets;
    unsigned int INDEX_MAX;
    ADDRINT **TABLE_IP;
    ADDRINT **TABLE_TARGET;
    unsigned int *TABLE_INDEX;
};

#endif
