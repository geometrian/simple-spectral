#include "spectrum.hpp"


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
