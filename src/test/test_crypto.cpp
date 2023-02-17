//
// Copyright (c) 2023 Julian Hinxlage. All rights reserved.
//

#include <iostream>
#include <vector>

#include "crypto/ecc.h"
#include "crypto/sha.h"

#include "util/hex.h"

int main(int argc, char* argv[]) {
	std::vector<std::string> strs = {
		"",
		"Hello World",
		"Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore et dolore magna aliquyam erat, sed diam voluptua. At vero eos et accusam et justo duo dolores et ea rebum. Stet clita kasd gubergren, no sea takimata sanctus est Lorem ipsum dolor sit amet. Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore et dolore magna aliquyam erat, sed diam voluptua. At vero eos et accusam et justo duo dolores et ea rebum. Stet clita kasd gubergren, no sea takimata sanctus est Lorem ipsum dolor sit amet.",
	};

	EccPrivateKey priv;
	EccPublicKey pub;
	eccGenerate(priv, pub);
	for (auto& str : strs) {

		printf("%s\n", str.c_str());

		{
			Blob<256> hash = sha256(str);
			std::string hex;
			toHex(hash, hex);
			printf("sha256: %s\n", hex.c_str());
		}
		{
			Blob<256> hash = sha3_256(str);
			std::string hex;
			toHex(hash, hex);
			printf("sha3_256: %s\n", hex.c_str());
		}
		{
			Blob<512> hash = sha512(str);
			std::string hex;
			toHex(hash, hex);
			printf("sha512: %s\n", hex.c_str());
		}
		{
			Blob<512> hash = sha3_512(str);
			std::string hex;
			toHex(hash, hex);
			printf("sha3_512: %s\n", hex.c_str());
		}
		{
			EccSignature sig = eccCreateSignature(str.c_str(), str.size(), priv);
			bool valid = eccVerifySignature(str.c_str(), str.size(), pub, sig);
			std::string hex;
			toHex(sig, hex);
			printf("sig: %s\n", hex.c_str());
			printf("valid: %i\n", (int)valid);
		}
		
		printf("\n\n");
	}
}
