#ifndef	_CLASS_SPECTRUM_
	#define	_CLASS_SPECTRUM_

	#include <vector>
	#include <complex>
	#include <cmath>


	#include "enum-field.h"
	#include "scalar/scalarField.h"
//	#include "scalar/varNQCD.h"

	class	SpecBin {

		private:

		std::vector<double>	binK;
		std::vector<double>	binG;
		std::vector<double>	binV;
		std::vector<double>	binP;
		std::vector<double>	binPS;

		std::vector<double>	cosTable;

		Scalar			*field;

		size_t			Lx, Ly, Lz, hLx, hLy, hLz, hTz, Tz, nPts, kMax, powMax;
		double			mass, massSax; // squared masses (comoving)
		double 			ztime, depta;
		double 			zaskar ;
		float				zaskarf ;
		std::complex<double> zaska ;
		std::complex<float> zaskaf ;

		void			fillCosTable ();

		const bool		spec;
		const FieldPrecision	fPrec;
		const FieldType		fType;

		public:

				SpecBin (Scalar *field, const bool spectral) : field(field), Ly(field->Length()), Lz(field->Depth()), Tz(field->TotalDepth()),
									       fPrec(field->Precision()), nPts(field->Size()), fType(field->Field()), spec(spectral) {
				kMax   = (Ly >=  Tz) ? (Ly>>1) : (Tz>>1);
				powMax = floor(sqrt(2.*(Ly>>1)*(Ly>>1) + (Tz>>1)*(Tz>>1)))+1;

				binK.resize(powMax); binK.assign(powMax, 0.);
				binG.resize(powMax); binG.assign(powMax, 0.);
				binV.resize(powMax); binV.assign(powMax, 0.);
				binP.resize(powMax); binP.assign(powMax, 0.);
				binPS.resize(powMax); binPS.assign(powMax, 0.);

				mass    = field->AxionMassSq()*(*field->zV())*(*field->zV());
				massSax = field->SaxionMassSq()*(*field->zV())*(*field->zV());
				ztime   = *field->zV();
				depta   = field->BckGnd()->PhysSize()/Ly;

				zaskar  = field->Saskia()*ztime;
				zaskarf = (float) zaskar ;
				zaska   = std::complex<double>(zaskar,0.);
				zaskaf  = std::complex<float>(zaskarf,0.f);

				fillCosTable();

				hLy = Ly >> 1;
				hLz = Lz >> 1;
				hTz = Tz >> 1;

				switch (fType) {
					// THIS CASE IS ILL DEFINED, WILL NEVER BE USED
					// well... I am starting to use it!
					// I assume the saxion mode will be analised also in real components
					case	FIELD_SAXION:
						// Lx   = Ly;
						// hLx  = Ly >> 1;
						// break;
					case	FIELD_AXION_MOD:
					case	FIELD_AXION:
						Lx   = (Ly >> 1)+1;
						hLx  = Lx;
						break;

					case	FIELD_WKB:
						LogError("Warning: WKB fields not supported for analysis");
						Lx = 0; Ly = 0; hLx = 0; nPts = 0;
						return;
						break;
				}

				nPts = Lx*Ly*Lz;

		}


		inline const size_t	PowMax() const { return powMax; }

		inline double		operator()(size_t idx, SpectrumType sType)	const;
		inline double&		operator()(size_t idx, SpectrumType sType);

		inline const double*	data(SpectrumType sType)	const;
		inline	     double*	data(SpectrumType sType);

		template<typename cFloat, const SpectrumType sType, const bool spectral>
		void	fillBins	();

		template<typename cFloat>
		void	filterFFT	(int neigh);

		void	nRun		();
		void	nSRun		();
		void	pRun		();
		void	nmodRun		();


		void	filter	(int neigh);

	};


	inline double	SpecBin::operator()(size_t idx, SpectrumType sType)	const	{

		switch(sType) {
			case	SPECTRUM_K:
			case	SPECTRUM_KS:
				return binK[idx];
				break;

			case	SPECTRUM_G:
			case	SPECTRUM_GS:
				return binG[idx];
				break;

			case	SPECTRUM_V:
			case	SPECTRUM_VS:
				return binV[idx];
				break;

			case	SPECTRUM_P:
				return binP[idx];
				break;

			case	SPECTRUM_PS:
				return binPS[idx];
				break;

		}
	}

	inline double&	SpecBin::operator()(size_t idx, SpectrumType rType)		{

		switch(rType) {
			case	SPECTRUM_K:
			case	SPECTRUM_KS:
				return binK[idx];
				break;

			case	SPECTRUM_G:
			case	SPECTRUM_GS:
				return binG[idx];
				break;

			case	SPECTRUM_V:
			case	SPECTRUM_VS:
				return binV[idx];
				break;

			case	SPECTRUM_P:
				return binP[idx];
				break;

			case	SPECTRUM_PS:
				return binPS[idx];
				break;

		}
	}

	inline double*	SpecBin::data(SpectrumType sType) {

		switch(sType) {
			case	SPECTRUM_K:
			case	SPECTRUM_KS:
				return binK.data();
				break;

			case	SPECTRUM_G:
			case	SPECTRUM_GS:
				return binG.data();
				break;

			case	SPECTRUM_V:
			case	SPECTRUM_VS:
				return binV.data();
				break;

			case	SPECTRUM_P:
				return binP.data();
				break;

			case	SPECTRUM_PS:
			case	SPECTRUM_NN:
				return binPS.data();
				break;

		}
	}

	inline const double*	SpecBin::data(SpectrumType sType)	const	{

		switch(sType) {
			case	SPECTRUM_K:
			case	SPECTRUM_KS:
				return binK.data();
				break;

			case	SPECTRUM_G:
			case	SPECTRUM_GS:
				return binG.data();
				break;

			case	SPECTRUM_V:
			case	SPECTRUM_VS:
				return binV.data();
				break;

			case	SPECTRUM_P:
				return binP.data();
				break;

			case	SPECTRUM_PS:
			case	SPECTRUM_NN:
				return binPS.data();
				break;
		}
	}
#endif
