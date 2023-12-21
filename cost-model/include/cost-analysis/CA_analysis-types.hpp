#ifndef MAESTRO_CA_ANALYSIS_TYPES_HPP_
#define MAESTRO_CA_ANALYSIS_TYPES_HPP_


namespace maestro {
    namespace CA {

        enum class IterationStatus {Init, Steady, Edge, Unroll, InitEdge, InitUnroll};
        enum class EstimationType {Min, Max, Exact, NumEstimationTypes};
        const int num_iter_status = 3;

    };
};
#endif
