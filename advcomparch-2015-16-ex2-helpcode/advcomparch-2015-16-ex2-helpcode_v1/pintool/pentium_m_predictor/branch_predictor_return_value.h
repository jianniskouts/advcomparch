#ifndef BRANCH_PREDICTOR_RETURN_VALUE
#define BRANCH_PREDICTOR_RETURN_VALUE

class BranchPredictorReturnValue {
public:
    enum BranchType {
        InvalidBranch = 0,
        DirectBranch,
        IndirectBranch,
        UnconditionalBranch,
        ConditionalBranch
    };

    bool prediction;
    bool hit;
    ADDRINT target;
    BranchType type;
};

#endif /* BRANCH_PREDICTOR_RETURN_VALUE */
