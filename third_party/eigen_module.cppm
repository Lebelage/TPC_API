module;
#include <Eigen/Dense>
export module tpc.third_party.eigen;
export namespace tpc::third_party::eigen {
    using Eigen::MatrixXd;
    using Eigen::VectorXd;
    using Eigen::JacobiSVD;
    using Eigen::ComputeThinU;
    using Eigen::ComputeThinV;
}
