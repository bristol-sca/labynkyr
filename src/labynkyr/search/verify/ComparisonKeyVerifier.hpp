/*
 * University of Bristol – Open Access Software Licence
 * Copyright (c) 2016, The University of Bristol, a chartered
 * corporation having Royal Charter number RC000648 and a charity
 * (number X1121) and its place of administration being at Senate
 * House, Tyndall Avenue, Bristol, BS8 1TH, United Kingdom.
 * All rights reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided
 * with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * Any use of the software for scientific publications or commercial
 * purposes should be reported to the University of Bristol
 * (OSI-notifications@bristol.ac.uk and quote reference 2514). This is
 * for impact and usage monitoring purposes only.
 *
 * Enquiries about further applications and development opportunities
 * are welcome. Please contact elisabeth.oswald@bristol.ac.uk
*/
/*
 * ComparisonKeyVerifier.hpp
 *
 */

#ifndef LABYNKYR_SRC_LABYNKYR_SEARCH_VERIFY_COMPARISONKEYVERIFIER_HPP_
#define LABYNKYR_SRC_LABYNKYR_SEARCH_VERIFY_COMPARISONKEYVERIFIER_HPP_

#include "labynkyr/search/verify/KeyVerifier.hpp"

#include <algorithm>
#include <stdexcept>

namespace labynkyr {
namespace search {

/**
 *
 * Implementation of KeyVerifier designed for benchmarking and testing purposes. The verifier is provided with the correct key,
 * and checks incoming candidates against the known key.
 *
 * @tparam KeyLenBits
 */
template<uint32_t KeyLenBits>
class ComparisonKeyVerifier : public KeyVerifier<KeyLenBits> {
public:
	/**
	 *
	 * @param key the known correct key value
	 */
	ComparisonKeyVerifier(Key<KeyLenBits> key)
	: KeyVerifier<KeyLenBits>()
	, key(key)
	, count(0)
	, keyFound(false)
	{
	}

	~ComparisonKeyVerifier() {}

	void checkKey(std::vector<uint8_t> const & candidateKeyBytes) override {
		bool const match = std::equal(
			key.asBytes().begin(),
			key.asBytes().end(),
			candidateKeyBytes.begin()
		);
		count++;
		keyFound |= match;
	}

	uint64_t keysChecked() const override {
		return count;
	}

	bool success() const override {
		return keyFound;
	}

	Key<KeyLenBits> correctKey() override {
		if(keyFound) {
			return key;
		}
		throw std::logic_error("Key has not been found");
	}

	void flush() override {}
private:
	Key<KeyLenBits> const key;
	uint64_t count;
	bool keyFound;
};

template<uint32_t KeyLenBits>
class ComparisonKeyVerifierFactory : public KeyVerifierFactory<KeyLenBits> {
public:
	ComparisonKeyVerifierFactory(Key<KeyLenBits> key)
	: KeyVerifierFactory<KeyLenBits>()
	, key(key)
	{
	}

	~ComparisonKeyVerifierFactory() {}

	std::unique_ptr<KeyVerifier<KeyLenBits>> newVerifier() const override {
		auto * verifier = new ComparisonKeyVerifier<KeyLenBits>(key);
		return std::unique_ptr<ComparisonKeyVerifier<KeyLenBits>>(verifier);
	}
private:
	Key<KeyLenBits> const key;
};

} /*namespace search */
} /*namespace labynkyr */

#endif /* LABYNKYR_SRC_LABYNKYR_SEARCH_VERIFY_COMPARISONKEYVERIFIER_HPP_ */
