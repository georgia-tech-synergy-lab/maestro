#ifndef MAESTRO_CA_ANALYSIS_TYPES_HPP_
#define MAESTRO_CA_ANALYSIS_TYPES_HPP_


namespace maestro {
    namespace CA {

        enum class IterationStatus {Init, Edge, Unroll};
        enum class EstimationType {Min, Max, Exact, NumEstimationTypes};

    };
};
#endif
