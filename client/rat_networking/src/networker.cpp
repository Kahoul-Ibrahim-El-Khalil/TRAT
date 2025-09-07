#include "rat/networking.hpp"

#include <chrono>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char *argv[]) {
		if(argc < 2) {
			std::cerr << "Usage:\n"
			          << "  simple_download <url>\n"
			          << "  download <url> <file>\n"
			          << "  upload <url> <file>\n";
			return 1;
		}

	rat::networking::Client curl_client; // <-- fixed: actual object

	std::string command = argv[1];

	auto start = std::chrono::steady_clock::now();

		if((argc == 3) && (command == "simple_download")) {
			const std::string url = argv[2];
			std::filesystem::path file_path =
			    rat::networking::_getFilePathFromUrl(url);

			auto ok = curl_client.download(url, file_path);

			auto end = std::chrono::steady_clock::now();
			auto duration =
			    std::chrono::duration_cast<std::chrono::milliseconds>(end -
			                                                          start)
			        .count();

				if(ok.curl_code == CURLE_OK) {
					std::cout << "Download SUCCESS → " << file_path << " ("
					          << duration << " ms)\n";
				}
				else {
					std::cerr << "Download FAILED\n";
				}
		}
		else if((argc == 4) && (command == "download")) {
			const std::string url = argv[2];
			const std::filesystem::path file_path = argv[3];

			auto ok = curl_client.download(url, file_path);

			auto end = std::chrono::steady_clock::now();
			auto duration =
			    std::chrono::duration_cast<std::chrono::milliseconds>(end -
			                                                          start)
			        .count();

				if(ok.curl_code == CURLE_OK) {
					std::cout << "Download SUCCESS → " << file_path << " ("
					          << duration << " ms)\n";
				}
				else {
					std::cerr << "Download FAILED\n";
				}
		}
		else if((argc == 4) && (command == "upload")) {
			const std::string url = argv[2];
			const std::filesystem::path file_path = argv[3];

			auto ok = curl_client.upload(file_path, url);

			auto end = std::chrono::steady_clock::now();
			auto duration =
			    std::chrono::duration_cast<std::chrono::milliseconds>(end -
			                                                          start)
			        .count();

				if(ok.curl_code == CURLE_OK) {
					std::cout << "Upload SUCCESS from " << file_path << " ("
					          << duration << " ms)\n";
				}
				else {
					std::cerr << "Upload FAILED\n";
				}
		}
		else {
			std::cerr << "Invalid arguments. Run without args to see usage.\n";
			return 1;
		}

	return 0;
}
#ifdef DEBUG_RAT_NETWORKING
#undef DEBUG_RAT_NETWORKING
#endif
