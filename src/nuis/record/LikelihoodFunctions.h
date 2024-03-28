#pragma once

#include "nuis/record/Comparison.h"

namespace nuis {
namespace likelihood {
// Fix these for full covariance estimates
double Chi2(const Comparison& fr) {
    Eigen::ArrayXd res = (fr.data[0].value - fr.estimate[0].value);
    res /= fr.data[0].error;

    Eigen::ArrayXd covar = fr.correlation;
    return (res * res).sum();
}

double Poisson(const Comparison& fr) {
    Eigen::ArrayXd res = (fr.data[0].value - fr.estimate[0].value);

    double chi2 = 0;
    for (long int i = 0; i < fr.data[0].value.rows(); i++){
        double dt = fr.data[0].value[i];
        double mc = fr.estimate[0].value[i];

        if (mc <= 0) continue; // From old NUISANCE, Is this correct?

        if (dt <= 0) {
            chi2 += 2 * (mc - dt);
        } else {
            chi2 += 2 * (mc - dt + (dt * log(dt / mc)));
        }

    }
    return chi2;
}

double SimpleResidual(const Comparison& fr) {
    Eigen::ArrayXd res = (fr.data[0].value - fr.estimate[0].value);
    res /= fr.data[0].error;
    return (res * res).sum();
}
}
}