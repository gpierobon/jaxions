#include <complex>
#include <cstring>

#include "scalar/scalarField.h"

using namespace std;

template<typename Float>
void	toThetaKernelXeon (Scalar *sField, const Float shift)
{
	LogMsg(VERB_NORMAL, "[2tX] To theta Xeon ... ");

	const size_t V  = sField->Size();
	const size_t S  = sField->Surf();
	const size_t Lz = sField->Depth();
	const size_t Go = 2*(V+S);

	Float *mField   = static_cast<Float*>(sField->mStart());
	Float *vField   = static_cast<Float*>(sField->vCpu());
	Float *aField1   = static_cast<Float*>(sField->mFrontGhost()) ;
	Float *aField2   = static_cast<Float*>(sField->mBackGhost()) ;
	complex<Float> *cmField  = static_cast<complex<Float>*>(sField->mStart());
	complex<Float> *cvField  = static_cast<complex<Float>*>(sField->vCpu());

	const Float z = static_cast<Float>(sField->zV()[0]);

	for (size_t cZ = 0; cZ < Lz; cZ++)
	{
		const size_t Vo = cZ*S;
		#pragma omp parallel for default(shared) schedule(static)
		for (size_t lpc = 0; lpc < S; lpc++)
		{
			complex<Float> mTmp = cmField[Vo+lpc] - complex<Float>(shift,0.);
			Float iMod      = z/(mTmp.real()*mTmp.real() + mTmp.imag()*mTmp.imag());
			aField1[lpc]   = arg(mTmp);
			aField2[lpc]   = (cvField[lpc]*conj(mTmp)).imag()*iMod + aField1[lpc];
			aField1[lpc]    *= z;
		}
		memcpy (mField + Vo, aField1, sizeof(Float)*S);
		memcpy (vField + Vo, aField2, sizeof(Float)*S);
	}
}




void	toThetaXeon (Scalar *sField, const double shift)
{
	switch (sField->Precision())
	{
		case FIELD_DOUBLE:

			toThetaKernelXeon<double> (sField, shift);
			break;

		case FIELD_SINGLE:

			toThetaKernelXeon<float> (sField, (float) shift);
			break;

		default:

			printf("Wrong precision\n");
			exit  (1);
			break;
	}
}
