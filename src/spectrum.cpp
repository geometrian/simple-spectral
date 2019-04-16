#include "spectrum.hpp"

#include "util/math-helpers.hpp"



#ifdef RENDER_MODE_SPECTRAL



_Spectrum::_Spectrum(float data) :
	_Spectrum( std::vector<float>(2_zu,data), LAMBDA_MIN,LAMBDA_MAX )
{}
_Spectrum::_Spectrum(std::vector<float> const& data, nm low,nm high) :
	_data(data), _low(low),_high(high)
{
	if (data.size()>=2); else {
		fprintf(stderr,"Must have at-least two elements in sampled spectrum!\n");
		throw -1;
	}

	nm    numer = _high - _low;
	float denom = static_cast<float>(data.size()-1);
	_delta_lambda       = numer / denom;
	_delta_lambda_recip = denom / numer;
}

//TODO: optimize!
float _Spectrum::_sample_nearest(nm lambda) const {
	float i_f = (lambda-_low)*_delta_lambda_recip;
	i_f = std::round(i_f);
	int i_i = static_cast<int>(i_f);
	if (i_i>=0) {
		size_t i_zu = static_cast<size_t>(i_i);
		if (i_zu<_data.size()) return _data[i_zu];
	}
	return 0.0f;
}
float _Spectrum::_sample_linear (nm lambda) const {
	//This could definitely be optimized.

	float i = (lambda-_low)*_delta_lambda_recip;
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
_Spectrum::HeroSample _Spectrum::operator[](nm lambda_0) const {
	HeroSample result;
	for (size_t i=0;i<SAMPLE_WAVELENGTHS;++i) {
		result[i] = std::invoke(_sampler, this, lambda_0+i*LAMBDA_STEP );
	}
	return result;
}

_Spectrum _Spectrum::operator*(float sc) const {
	_Spectrum result = *this;
	for (float& f : result._data) f*=sc;
	return result;
}
_Spectrum _Spectrum::operator*(_Spectrum const& other) const {
	nm low  = std::max(_low, other._low );
	nm high = std::min(_high,other._high);

	//TODO: other cases are valid, but not implemented!
	assert(
		_delta_lambda == other._delta_lambda &&
		std::fmod(       _low -low,        _delta_lambda )==0.0f &&
		std::fmod( other._low -low,  other._delta_lambda )==0.0f &&
		std::fmod(       _high-high,       _delta_lambda )==0.0f &&
		std::fmod( other._high-high, other._delta_lambda )==0.0f
	);

	std::vector<float> data;
	data.resize(static_cast<size_t>( (high-low)/_delta_lambda + 1 ));
	for (size_t i=0;i<data.size();++i) {
		nm lambda = low + _delta_lambda*static_cast<float>(i);
		data[i] = _sample_nearest(lambda) * other._sample_nearest(lambda);
	}

	return _Spectrum( data, low,high );
}
_Spectrum _Spectrum::operator+(_Spectrum const& other) const {
	nm low  = std::max(_low, other._low );
	nm high = std::min(_high,other._high);

	//TODO: other cases are valid, but not implemented!
	assert(
		_delta_lambda == other._delta_lambda &&
		std::fmod(       _low -low,        _delta_lambda )==0.0f &&
		std::fmod( other._low -low,  other._delta_lambda )==0.0f &&
		std::fmod(       _high-high,       _delta_lambda )==0.0f &&
		std::fmod( other._high-high, other._delta_lambda )==0.0f
	);

	std::vector<float> data;
	data.resize(static_cast<size_t>( (high-low)/_delta_lambda + 1 ));
	for (size_t i=0;i<data.size();++i) {
		nm lambda = low + _delta_lambda*static_cast<float>(i);
		data[i] = _sample_nearest(lambda) + other._sample_nearest(lambda);
	}

	return _Spectrum( data, low,high );
}

float _Spectrum::integrate(_Spectrum const& spec                         ) {
	float result = 0.0f;

	//The midpoint-rule Riemann sum is exact for both nearest and linear reconstruction.
	#if 0
		//Sum of boxes
		for (float const& value : spec._data) result+=value*spec._delta_lambda;
	#else
		//Mathematically the same, but faster
		for (float const& value : spec._data) result+=value;
		result *= spec._delta_lambda;
	#endif

	return result;
}
float _Spectrum::integrate(_Spectrum const& spec0, _Spectrum const& spec1) {
	//First, find the set of unique sample locations on which both spectra are defined, plus their
	//	next points outward (i.e. where they are guaranteed to be zero).

	nm low =  std::max( spec0._low -spec0._delta_lambda, spec1._low -spec1._delta_lambda );
	nm high = std::min( spec0._high+spec0._delta_lambda, spec1._high+spec1._delta_lambda );

	std::set<nm> sample_pts_set;
	auto add_sample_points = [&](_Spectrum const& spec) -> void {
		nm sample = spec._low - spec._delta_lambda;
		while (sample< low )                                  sample+=spec._delta_lambda;
		while (sample<=high) { sample_pts_set.insert(sample); sample+=spec._delta_lambda; }
	};
	add_sample_points(spec0);
	add_sample_points(spec1);

	std::vector<nm> sample_pts(sample_pts_set.cbegin(),sample_pts_set.cend());
	std::sort(sample_pts.begin(),sample_pts.end());

	//Integrate by trapezoidal rule, which will be exact regardless of whether the functions are
	//	taken to be defined with nearest or linear reconstruction, since the functions touch every
	//	sample point of both spectra.
	float result = 0.0f;
	for (size_t i=0;i<sample_pts.size()-1;++i) {
		nm const& lambda_low  = sample_pts[i  ];
		nm const& lambda_high = sample_pts[i+1];
		assert(lambda_high>lambda_low);

		float val0low  = spec0._sample_linear(lambda_low );
		float val1low  = spec1._sample_linear(lambda_low );
		float val0high = spec0._sample_linear(lambda_high);
		float val1high = spec1._sample_linear(lambda_high);
		float vallow  = val0low  * val1low;
		float valhigh = val0high * val1high;

		result += 0.5f*(vallow+valhigh) * (lambda_high-lambda_low);
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



#endif
