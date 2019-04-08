#include "spectrum.hpp"

#include "math-helpers.hpp"


Spectrum::Spectrum(float data) :
	Spectrum( std::vector<float>( size_t(2), data ), LAMBDA_MIN,LAMBDA_MAX )
{}
Spectrum::Spectrum(std::vector<float> const& data, nm low,nm high) :
	_data(data), _low(low),_high(high)
{
	if (data.size()>=2); else {
		fprintf(stderr,"Must have at-least two elements in sampled spectrum!\n");
		throw -1;
	}

	_sc = static_cast<float>(data.size()-1) / (_high-_low);
}

float Spectrum::_sample_nearest(nm lambda) const {
	float i_f = (lambda-_low)*_sc;
	i_f = std::round(i_f);
	int i_i = static_cast<int>(i_f);
	if (i_i>=0) {
		size_t i_zu = static_cast<size_t>(i_i);
		if (i_zu<_data.size()) return _data[i_zu];
	}
	return 0.0f;
}
float Spectrum::_sample_linear (nm lambda) const {
	//This could definitely be optimized.

	float i = (lambda-_low)*_sc;
	float i0f = std::floor(i);
	float frac = i - i0f;
	int i0 = static_cast<int>(i0f);
	int i1 = i0 + 1;

	float val0;
	if (i0>=0&&i0<_data.size()) {
		val0 = _data[static_cast<size_t>(i0)];
	} else {
		val0 = 0.0f;
	}
	float val1;
	if (i1>=0&&i1<_data.size()) {
		val1 = _data[static_cast<size_t>(i1)];
	} else {
		val1 = 0.0f;
	}

	return Math::lerp( val0,val1, frac );
}
Spectrum::Sample Spectrum::operator[](nm lambda_0) const {
	Sample result;
	for (size_t i=0;i<SAMPLE_WAVELENGTHS;++i) {
		result[i] = std::invoke(_sampler, this, lambda_0+i*LAMBDA_STEP );
	}
	return result;
}

float Spectrum::integrate(Spectrum const& spec) {
	float result = 0.0f;
	nm step = 1.0f / spec._sc;
	nm lambda = spec._low;
	while (lambda<=spec._high) {
		result += spec._sample_linear(lambda);
		lambda += step;
	}
	return result;
}
float Spectrum::integrate(Spectrum const& spec0, Spectrum const& spec1) {
	//Integrate by trapezoidal rule, which will be exact since the functions are defined piecewise-
	//	linearly, and we touch every sample point of both spectra.


	nm low =  std::max(spec0._low ,spec1._low );
	nm high = std::min(spec0._high,spec1._high);


	std::set<nm> sample_pts_set;

	nm sample;
	sample = spec0._low;
	nm step0 = 1.0f / spec0._sc;
	while (sample< low )                                  sample+=step0;
	while (sample<=high) { sample_pts_set.insert(sample); sample+=step0; }

	sample = spec1._low;
	nm step1 = 1.0f / spec1._sc;
	while (sample< low )                                  sample+=step1;
	while (sample<=high) { sample_pts_set.insert(sample); sample+=step1; }


	std::vector<nm> sample_pts(sample_pts_set.cbegin(),sample_pts_set.cend());

	float result = 0.0f;
	/*for (size_t i=0;i<sample_pts.size()-1;++i) {
		nm const& lambda_low  = sample_pts[i  ];
		nm const& lambda_high = sample_pts[i+1];
		assert(lambda_high>lambda_low);

		float val0low  = spec0._sample_linear(lambda_low );
		float val1low  = spec1._sample_linear(lambda_low );
		float val0high = spec0._sample_linear(lambda_high);
		float val1high = spec1._sample_linear(lambda_high);
		float val0 = val0low * val0high;
		float val1 = val1low * val1high;

		result += 0.5f*(val0+val1);// * (lambda_high-lambda_low);
	}
	//result /= high - low;*/
	for (nm const& lambda : sample_pts) {
		float val0 = spec0._sample_linear(lambda);
		float val1 = spec1._sample_linear(lambda);
		result += val0*val1;
	}

	return result;
}


std::vector<std::vector<float>> load_spectral_data(std::string const& csv_path) {
	std::ifstream file(csv_path);
	if (file.good()); else {
		fprintf(stderr,"Could not open required file \"%s\"!\n",csv_path.c_str());
		throw -1;
	}

	std::vector<std::vector<float>> data;

	std::string line;
	while (std::getline(file, line)) {
		std::istringstream ss(line);

		for (size_t i=0;;++i) {
			float f;
			if (ss >> f) {
				if (i==data.size()) data.emplace_back();
				data[i].push_back(f);
			} else {
				fprintf(stderr,"Expected number when parsing file!\n");
				throw -2;
			}

			char c;
			if (!(ss>>c)) break;
		}
	}

	for (size_t i=1;i<data.size();++i) {
		if (data[i].size()==data[0].size()); else {
			fprintf(stderr,"Data dimension mismatch in file!\n");
			throw -3;
		}
	}

	return data;
}
