/*
Copyright (c) 2016-2017, UT-Battelle, LLC

MERA++, Version 0.

This file is part of MERA++.
MERA++ is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
MERA++ is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with MERA++. If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef MERA_BUILDER2D_H
#define MERA_BUILDER2D_H
#include "BuilderBase.h"
#include "TensorSrep.h"

namespace Mera {

class Builder2D : public BuilderBase {

	static const SizeType DIMENSION = 2;

public:

	typedef TensorSrep::VectorSizeType VectorSizeType;
	typedef TensorSrep::VectorPairSizeType VectorPairSizeType;
	typedef TensorSrep::PairSizeType PairSizeType;

	Builder2D(SizeType sites, SizeType arity, bool isPeriodic)
	    : sites_(sites), srep_("")
	{
		if (arity != 4)
			throw PsimagLite::RuntimeError("MeraBuilder2D: arity must be 4 for now\n");

		SizeType x = sqrt(sites);
		if (x*x != sites) sitesNotSupported();
		if (x & 1) sitesNotSupported();
		x /= 2;
		if (x & 1) sitesNotSupported();

		SizeType tensors = sites/4;
		SizeType counter = 0;
		SizeType summed = 0;
		SizeType savedSummedForU = 0;
		SizeType idsU = 0;
		SizeType idsW = 0;
		while (tensors > 1) {
			SizeType savedSummedForW = summed;
			createUlayer(summed,
			             idsU,
			             savedSummedForU,
			             tensors,
			             counter);
			savedSummedForU = summed;
			createWlayer(summed,
			             idsW,
			             savedSummedForW,
			             tensors);
			tensors /= 4;
			counter++;
		}

		assert(summed > 4);
		summed -= 4;
		PsimagLite::String rArgs("");
		for (SizeType i = 0; i < 4; ++i) {
			rArgs += "s" + ttos(summed++);
			if (i < 3) rArgs += ",";
		}

		srep_ += "r0(" + rArgs + ")";
	}

	const PsimagLite::String& srep() const { return srep_; }


	TensorSrep* buildEnergyTerm(SizeType c,
	                            const TensorSrep& tensorSrep) const
	{
		SizeType connections = DIMENSION*sites_;
		div_t q = div(c, DIMENSION);
		SizeType direction = q.rem;
		SizeType site = q.quot;
		SizeType sitep = findSitePrime(site,direction);

		TensorSrep tensorSrep2(tensorSrep);
		tensorSrep2.conjugate();
		PsimagLite::String str3("h");
		str3 += ttos(c) + "(";
		str3 += "f" + ttos(connections) + ",";
		str3 += "f" + ttos(connections + 1) + "|";
		str3 += "f" + ttos(site) + ",";
		str3 += "f" + ttos(sitep) + ")";
		TensorSrep tensorSrep3(str3);
		TensorSrep::VectorSizeType indicesToContract(2,site);
		indicesToContract[1] = sitep;
		TensorSrep* tensorSrep4 = new TensorSrep(tensorSrep);
		tensorSrep4->contract(tensorSrep3,indicesToContract);
		if (!tensorSrep4->isValid(true))
			throw PsimagLite::RuntimeError("Invalid tensor\n");
		//correctFreeIndicesBeforeContraction(*tensorSrep4, site);

		std::cerr<<"LOWER"<<site<<"="<<tensorSrep2.sRep()<<"\n";
		std::cerr<<"UPPER"<<site<<"="<<tensorSrep4->sRep()<<"\n";
		tensorSrep4->contract(tensorSrep2);
		//std::cout<<"e"<<site<<"()="<<tensorSrep4->sRep()<<"\n";
		if (!tensorSrep4->isValid(true))
			throw PsimagLite::RuntimeError("Invalid tensor\n");
		return tensorSrep4;
	}

private:

	void sitesNotSupported() const
	{
		throw PsimagLite::RuntimeError("Builder2D: Requires sites == (4*x)^2 for x integer\n");
	}

	SizeType findSitePrime(SizeType site,SizeType direction) const
	{
		SizeType l = sqrt(sites_);
		assert(l*l == sites_);
		div_t q = div(site, l);
		int x = q.rem;
		int y = q.quot;
		if (direction == 0)
			return snapBack(x + 1, l) + y*l;
		return x + snapBack(y + 1, l);
	}

	void createUlayer(SizeType& summed,
	                  SizeType& idsU,
	                  SizeType savedSummed,
	                  SizeType n,
	                  SizeType counter)
	{
		VectorSizeType assignment(4);
		SizeType sqrtN = sqrt(n);
		assert(sqrtN*sqrtN == n);
		PsimagLite::String fOrS = (counter == 0) ? "f" : "s";
		for (SizeType i = 0; i < n; ++i) {
			getUAssignment(assignment, i, sqrtN);
			assert(assignment.size() == 4);

			PsimagLite::String inArgs("");
			for (SizeType i = 0; i < 4; ++i) {
				SizeType tmp = assignment[i];
				if (counter > 0) tmp += savedSummed;
				inArgs += fOrS + ttos(tmp);
				if (i < 3) inArgs += ",";
			}

			srep_ += "u" + ttos(idsU++) + "(" + inArgs + "|";

			PsimagLite::String outArgs("");
			for (SizeType i = 0; i < 4; ++i) {
				SizeType tmp = assignment[i];
				if (counter > 0) tmp += summed;
				outArgs += "s" + ttos(tmp);
				if (i < 3) outArgs += ",";
			}

			srep_ += outArgs + ")";
		}

		summed += 4*n;
	}

	void getUAssignment(VectorSizeType& assignment,
	                    SizeType ind,
	                    SizeType l) const
	{
		div_t q = div(ind, l);
		SizeType x = 2*q.rem;
		SizeType y = 2*q.quot;
		SizeType counter = 0;
		SizeType n = 2*l;
		assignment[counter++] = x + y*n;
		assignment[counter++] = x + 1 + y*n;
		assignment[counter++] = x + (y+1)*n;
		assignment[counter++] = x + 1 + (y+1)*n;
	}

	void createWlayer(SizeType& summed,
	                  SizeType& idsW,
	                  SizeType savedSummed,
	                  SizeType n)
	{
		SizeType sqrtN = sqrt(n);
		assert(sqrtN*sqrtN == n);
		VectorSizeType assignment(4, 0);
		for (SizeType i = 0; i < n; ++i) {
			getWAssignment(assignment, i, sqrtN);
			assert(assignment.size() == 4);

			PsimagLite::String inArgs("");
			for (SizeType i = 0; i < 4; ++i) {
				SizeType tmp = assignment[i] + savedSummed;
				inArgs += "s" + ttos(tmp);
				if (i < 3) inArgs += ",";
			}

			srep_ += "w" + ttos(idsW++) +"(" + inArgs + "|";

			srep_ += "s" + ttos(summed++) + ")";
		}
	}

	void getWAssignment(VectorSizeType& assignment,
	                    SizeType ind,
	                    SizeType l) const
	{
		div_t q = div(ind, l);
		int x = 2*q.rem;
		int y = 2*q.quot;
		SizeType counter = 0;
		int n = 2*l;
		SizeType xm1 = snapBack(x - 1, n);
		SizeType ym1 = snapBack(y - 1, n);

		assignment[counter++] = xm1 + ym1*n;
		assignment[counter++] = x + ym1*n;
		assignment[counter++] = xm1 + y*n;
		assignment[counter++] = x + y*n;
	}

	SizeType snapBack(int x, int l) const
	{
		if (x >= 0 && x < l) return x;
		while (x < 0) x += l;
		while (x >= l) x -= l;
		return x;
	}

	void correctFreeIndicesBeforeContraction(TensorSrep& t,
	                                         SizeType site) const
	{
		if (site < 1) return;

		SizeType x = (site + 1 == sites_) ? 0 : site + 1;
		t.swapFree(1,x);
		t.swapFree(0,site);
		t.refresh();

		SizeType max = site/2;
		if (site & 1) ++max;
		for (SizeType i = 0; i < t.size(); ++i) {
			TensorStanza& stanza = t(i);
			if (stanza.name() != "u") continue;
			SizeType id = stanza.id();
			if (id > max) continue;
			SizeType ins = stanza.ins();
			SizeType k = 0;
			VectorPairSizeType replacements;
			for (SizeType j = 0; j < ins; ++j) {
				if (stanza.legType(j) != TensorStanza::INDEX_TYPE_FREE)
					continue;
				SizeType legTag = stanza.legTag(j);
				SizeType shouldBe = 2*id + k;
				if (shouldBe > site) break;
				if (legTag != shouldBe)
					replacements.push_back(PairSizeType(legTag, shouldBe));
				++k;
			}

			stanza.replaceSummedOrFrees(replacements, 'f');
			stanza.refresh();
		}

		t.refresh();

		if (site + 1 == sites_) {
			t.swapFree(0,site);
			t.swapFree(1,site);
			t.refresh();
		}

		t.isValid(true);
	}

	SizeType sites_;
	PsimagLite::String srep_;
};
}
#endif // MERA_BUILDER2D_H
