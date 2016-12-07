/*
Copyright (c) 2016, UT-Battelle, LLC

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
#include "TensorEval.h"
#include "Vector.h"

typedef Mera::TensorStanza::VectorSizeType VectorSizeType;

void getAllStags(VectorSizeType& v0,
                 const Mera::TensorStanza& stanza0)
{
	Mera::TensorStanza::IndexDirectionEnum in = Mera::TensorStanza::INDEX_DIR_IN;
	Mera::TensorStanza::IndexDirectionEnum out = Mera::TensorStanza::INDEX_DIR_OUT;
	for (SizeType i = 0; i < stanza0.ins(); ++i) {
		if (stanza0.legType(i,in) != Mera::TensorStanza::INDEX_TYPE_SUMMED)
			continue;
		v0.push_back(stanza0.legTag(i,in));
	}

	for (SizeType i = 0; i < stanza0.outs(); ++i) {
		if (stanza0.legType(i,out) != Mera::TensorStanza::INDEX_TYPE_SUMMED)
			continue;
		v0.push_back(stanza0.legTag(i,out));
	}
}

PsimagLite::String stringT0Part(const VectorSizeType& setS,
                                const Mera::TensorStanza& stanza)
{
	Mera::TensorStanza::IndexDirectionEnum in = Mera::TensorStanza::INDEX_DIR_IN;
	Mera::TensorStanza::IndexDirectionEnum out = Mera::TensorStanza::INDEX_DIR_OUT;
	PsimagLite::String str = stanza.name() + ttos(stanza.id());
	SizeType counter = 0;
	for (SizeType i = 0; i < stanza.ins(); ++i) {
		Mera::TensorStanza::IndexTypeEnum legType = stanza.legType(i,in);
		SizeType legTag = stanza.legTag(i,in);
		bool isSummed = (legType == Mera::TensorStanza::INDEX_TYPE_SUMMED);
		bool notInSet = (isSummed && std::find(setS.begin(),setS.end(),legTag) == setS.end());
		if (!isSummed || notInSet) {
			if (counter++ == 0)
				str += "(";
			else
				str += ",";
			str += Mera::TensorStanza::indexTypeToString(legType) + ttos(legTag);
		}
	}

	if (counter == 0) str += "(";
	counter = 0;
	for (SizeType i = 0; i < stanza.outs(); ++i) {
		Mera::TensorStanza::IndexTypeEnum legType = stanza.legType(i,out);
		SizeType legTag = stanza.legTag(i,out);
		bool isSummed = (legType == Mera::TensorStanza::INDEX_TYPE_SUMMED);
		bool notInSet = (isSummed && std::find(setS.begin(),setS.end(),legTag) == setS.end());
		if (!isSummed || notInSet) {
			if (counter++ == 0)
				str += "|";
			else
				str += ",";
			str += Mera::TensorStanza::indexTypeToString(legType) + ttos(legTag);
		}
	}

	return str + ")";
}

void breakUpTensor(PsimagLite::String str, SizeType ind, SizeType jnd)
{
	Mera::TensorSrep srep(str);
	Mera::TensorStanza stanza0 = srep(ind);
	Mera::TensorStanza stanza1 = srep(jnd);
	std::cerr<<"We're going to break tensor ";
	PsimagLite::String c = (stanza0.isConjugate()) ? "*" : "";
	std::cerr<<stanza0.name()<<stanza0.id()<<c<<" and ";
	c = (stanza1.isConjugate()) ? "*" : "";
	std::cerr<<stanza1.name()<<stanza1.id()<<c;
	std::cerr<<" from "<<str<<"\n";

	// find commont setS
	VectorSizeType v0;
	getAllStags(v0,stanza0);
	VectorSizeType v1;
	getAllStags(v1,stanza1);

	VectorSizeType setS;
	for (SizeType i = 0; i < v0.size(); ++i)
		for (SizeType j = 0; j < v1.size(); ++j)
			if (v0[i] == v1[j]) setS.push_back(v0[i]);

	if (setS.size() == 0) return;

	// build t0
	PsimagLite::String str0 = stringT0Part(setS,stanza0);
	PsimagLite::String str1 = stringT0Part(setS,stanza1);
	std::cout<<"t0= "<<str0<<str1<<"\n";
}

int main()
{
	PsimagLite::String str = "u0(s0,s1|f0)h0(s0,s1|s2,s3)u0*(s2,s3|f1)";

	typedef Mera::TensorEval<double> TensorEvalType;
	typedef TensorEvalType::TensorType TensorType;
	TensorEvalType::VectorTensorType vt(2);

	TensorEvalType::VectorSizeType dimsu0(3,2);
	dimsu0[2] = 4;
	vt[0] = new TensorType(dimsu0,2);
	vt[0]->setToIdentity(1.0);

	TensorEvalType::VectorSizeType dimsh0(4,2);
	vt[1] = new TensorType(dimsh0,2);
	vt[1]->setToIdentity(1.0);

	TensorEvalType::VectorPairStringSizeType idNames;
	idNames.push_back(TensorEvalType::PairStringSizeType("u",0));
	idNames.push_back(TensorEvalType::PairStringSizeType("h",0));
	TensorEvalType::MapPairStringSizeType nameIdTensor;
	nameIdTensor[idNames[0]] = 0;
	nameIdTensor[idNames[1]] = 1;

	breakUpTensor(str,0,1);

	//	TensorEvalType tensorEval(str,vt,idNames,nameIdTensor);

	//	TensorEvalType::VectorSizeType freeIndices(2,0);
	//	freeIndices[1] = 0;
	//	std::cout<<tensorEval(freeIndices)<<"\n";

	for (SizeType i = 0; i < vt.size(); ++i) {
		delete vt[i];
		vt[i] = 0;
	}
}