#ifndef PARAMETERSFORSOLVER_H
#define PARAMETERSFORSOLVER_H

namespace Mera {

struct ParametersForSolver {

	ParametersForSolver()
	    : tauMax(3),numberOfSites(35)
	{}

	SizeType tauMax;
	SizeType numberOfSites;
}; // struct ParametersForSolver
} // namespace Mera
#endif // PARAMETERSFORSOLVER_H
