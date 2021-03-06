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
 * CandidateKeyForest.hpp
 *
 */

#ifndef LABYNKYR_SRC_LABYNKYR_SEARCH_ENUMERATE_CANDIDATEKEYFOREST_HPP_
#define LABYNKYR_SRC_LABYNKYR_SEARCH_ENUMERATE_CANDIDATEKEYFOREST_HPP_

#include "labynkyr/search/enumerate/CandidateKeyTree.hpp"
#include "labynkyr/search/verify/KeyVerifier.hpp"

#include "labynkyr/Key.hpp"

#include <forward_list>

namespace labynkyr {
namespace search {

/**
 *
 * The ActiveNodeFinder/Forest algorithm stores candidate keys in a tree.  This trades off memory at the cost of some computation when keys
 * are built to be verified.
 *
 * @tparam VecCount the number of distinguishing vectors in the attack (e.g 16 for SubBytes attacks on an AES-128 key)
 * @tparam VecLenBits the number bits of the key targeted by each subkey recovery attack (e.g 8 for SubBytes attacks on an AES-128 key)
 * @tparam SubkeyType the integer type used to store a subkey valyue (e.g uint8_t for a typical 8-bit DPA attack)
 */
template<uint32_t VecCount, uint32_t VecLenBits, typename SubkeyType>
class CandidateKeyForest {
public:
	enum {
		KeyLenBits = VecCount * VecLenBits
	};

	CandidateKeyForest(uint64_t forestSize)
	: forestSize(forestSize)
	{
		forest = std::make_shared<std::vector<std::unique_ptr<CandidateKeyTree<VecCount, VecLenBits, SubkeyType>>>>();
	}

	~CandidateKeyForest() {}

	/**
	 *
	 * Call to verify all candidate keys stored within the forest.
	 *
	 * @param verifier
	 */
	void verifyKeys(KeyVerifier<KeyLenBits> & verifier) {
		std::vector<SubkeyType> keyValues(VecCount);
		for(auto & tree : *forest.get()) {
			if(tree->size() > 0 && !verifier.success()) {
				uint32_t const keyBitLength = VecCount * VecLenBits;
				uint32_t const byteCount = (keyBitLength % 8 != 0) ? (keyBitLength / 8) + 1 : keyBitLength / 8;
				std::vector<uint8_t> keyBytes(byteCount);
				std::vector<SubkeyType> keyValues(VecCount);

				tree->buildAndVerifyKeys(keyValues, keyBytes, 0, verifier);
			}
		}
	}

	/**
	 *
	 * @return the total number of candidate keys stored in the forest
	 */
	uint64_t size() const {
		return forestSize;
	}

	/**
	 *
	 * Update this forest by merging in the candidates from a second forest, and given the next subkey value to use.
	 *
	 * @param other
	 * @param nextValue
	 */
	void merge(CandidateKeyForest<VecCount, VecLenBits, SubkeyType> const & other, SubkeyType nextValue) {
		if(other.size() > 0) {
			auto * newTree = new CandidateKeyTree<VecCount, VecLenBits, SubkeyType>(nextValue, other.getForest(), other.size());
			std::unique_ptr<CandidateKeyTree<VecCount, VecLenBits, SubkeyType>> newTreePtr(newTree);
			forest->push_back(std::move(newTreePtr));
			forestSize += newTree->size();
		}
	}

	/**
	 *
	 * Verify the candidate keys that would be generated by merging this forest with a second forest, and given the next subkey value to use.
	 * This forest will not be modified, as the final set of merges required by the enumeration algorithm do not need to be re-used.
	 *
	 * @param verifier
	 * @param other
	 * @param nextValue
	 */
	void verifyMergeCandidates(KeyVerifier<KeyLenBits> & verifier, CandidateKeyForest<VecCount, VecLenBits, SubkeyType> const & other, SubkeyType nextValue) const {
		if(other.size() > 0) {
			CandidateKeyTree<VecCount, VecLenBits, SubkeyType> mergeTree(nextValue, other.getForest(), other.size());

			uint32_t const byteCount = (KeyLenBits % 8 != 0) ? (KeyLenBits / 8) + 1 : KeyLenBits / 8;
			std::vector<uint8_t> keyBytes(byteCount);
			std::vector<SubkeyType> keyValues(VecCount);
			if(mergeTree.size() > 0) {
				mergeTree.buildAndVerifyKeys(keyValues, keyBytes, 0, verifier);
			}
		}
	}

	std::shared_ptr<std::vector<std::unique_ptr<CandidateKeyTree<VecCount, VecLenBits, SubkeyType>>>> getForest() const {
		return forest;
	}

	static std::unique_ptr<CandidateKeyForest<VecCount, VecLenBits, SubkeyType>> emptySet() {
		auto * forest = new CandidateKeyForest<VecCount, VecLenBits, SubkeyType>(0);
		return std::unique_ptr<CandidateKeyForest<VecCount, VecLenBits, SubkeyType>>(forest);
	}

	static std::unique_ptr<CandidateKeyForest<VecCount, VecLenBits, SubkeyType>> rejectStateSet() {
		return acceptStateSet();
	}

	static std::unique_ptr<CandidateKeyForest<VecCount, VecLenBits, SubkeyType>> acceptStateSet() {
		auto * forest = new CandidateKeyForest<VecCount, VecLenBits, SubkeyType>(1);
		return std::unique_ptr<CandidateKeyForest<VecCount, VecLenBits, SubkeyType>>(forest);
	}
private:
	std::shared_ptr<std::vector<std::unique_ptr<CandidateKeyTree<VecCount, VecLenBits, SubkeyType>>>> forest;
	uint64_t forestSize;
};


} /*namespace search */
} /*namespace labynkyr */

#endif /* LABYNKYR_SRC_LABYNKYR_SEARCH_ENUMERATE_CANDIDATEKEYFOREST_HPP_ */
