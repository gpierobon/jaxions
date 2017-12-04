#ifndef	_PROPAGATOR_
	#define	_PROPAGATOR_

	#include "scalar/scalarField.h"

	void	initPropagator	(PropType pType, Scalar *field, const double nQcd=7., const double delta=0.0, const double LL=15000., const double gm=0.1, VqcdType pot=VQCD_1);
	void	propagate	(Scalar *field, const double dz);
	void	resetPropagator	(Scalar *field);
	void	tunePropagator	(Scalar *field);
#endif
