#ifndef PENTIUM_M_BRANCH_PREDICTOR_H
#define PENTIUM_M_BRANCH_PREDICTOR_H

#include "../branch_predictor.h"
#include "branch_predictor_return_value.h"
#include "pentium_m_global_predictor.h"
#include "pentium_m_branch_target_buffer.h"
#include "pentium_m_bimodal_table.h"
#include "pentium_m_loop_branch_predictor.h"

#include <vector>

class PentiumMBranchPredictor : public BranchPredictor
{
public:
	PentiumMBranchPredictor() 
	    : BranchPredictor(), m_pir(0), m_last_gp_hit(0), m_last_lpb_hit(0) {};
	~PentiumMBranchPredictor() {};

    virtual bool predict(ADDRINT ip, ADDRINT target);
    virtual void update(bool predicted, bool actual, ADDRINT ip, ADDRINT target);
    virtual string getName()  { return "Pentium-M"; }

private:

	void update_pir(bool actual, ADDRINT ip, ADDRINT target,
	                BranchPredictorReturnValue::BranchType branch_type);
	ADDRINT hash_function(ADDRINT ip, ADDRINT pir);

	PentiumMGlobalPredictor m_global_predictor;
	PentiumMBranchTargetBuffer m_btb;
	PentiumMBimodalTable m_bimodal_table;
	PentiumMLoopBranchPredictor m_lpb;

	ADDRINT m_pir;

	bool m_last_gp_hit;
	bool m_last_lpb_hit;

};

bool PentiumMBranchPredictor::predict(ADDRINT ip, ADDRINT target)
{
	BranchPredictorReturnValue global_pred_out = m_global_predictor.lookup(ip, target, m_pir);
	BranchPredictorReturnValue btb_out = m_btb.lookup(ip, target);
	BranchPredictorReturnValue lpb_out = m_lpb.lookup(ip, target);
	bool bimodal_out = m_bimodal_table.predict(ip, target);

	m_last_gp_hit = global_pred_out.hit;
	m_last_lpb_hit = lpb_out.hit & btb_out.hit;

	// Outcome prediction logic
	bool result;
	if (global_pred_out.hit)
		result = global_pred_out.prediction;
	else if (lpb_out.hit & btb_out.hit)
		result = lpb_out.prediction;
	else
		result = bimodal_out;

	// TODO FIXME: Failed matches against the target address should force a
	//             branch or fetch miss

	return result;
}

void PentiumMBranchPredictor::update(bool predicted, bool actual,
                                             ADDRINT ip, ADDRINT target)
{
	updateCounters(predicted, actual);
	m_btb.update(predicted, actual, ip, target);
	m_lpb.update(predicted, actual, ip, target);

	// Update bimodal predictor only when global and loop predictors missed
	if (!m_last_gp_hit && !m_last_lpb_hit)
		m_bimodal_table.update(predicted, actual, ip, target);

	// Update global predictor only when other predictors were wrong
	if (predicted != actual)
		m_global_predictor.update(predicted, actual, ip, target, m_pir);

	// TODO FIXME: Properly propagate the branch type information from the 
	//             decoder (IndirectBranch information)
	update_pir(actual, ip, target, BranchPredictorReturnValue::ConditionalBranch);
}

void PentiumMBranchPredictor::update_pir(bool actual, ADDRINT ip, ADDRINT target,
                            BranchPredictorReturnValue::BranchType branch_type)
{
	ADDRINT rhs;

	if ((branch_type == BranchPredictorReturnValue::ConditionalBranch) & actual)
		rhs = ip >> 4;
	else if (branch_type == BranchPredictorReturnValue::IndirectBranch)
		rhs = (ip >> 4) | target;
	else
		// No PIR update
		return;

	m_pir = ((m_pir << 2) ^ rhs) & 0x7fff;
}

#endif
