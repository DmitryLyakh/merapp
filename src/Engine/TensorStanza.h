#ifndef TENSORSTANZA_H
#define TENSORSTANZA_H
#include "Vector.h"
#include "Tokenizer.h"
#include "TypeToString.h"
#include <cassert>
#include <iostream>
#include <algorithm>

namespace Mera {

class TensorStanza {

	typedef std::pair<char,SizeType> PairCharSizeType;
	typedef PsimagLite::Vector<PairCharSizeType>::Type VectorPairCharSizeType;

public:

	typedef PsimagLite::Vector<PsimagLite::String>::Type VectorStringType;

	enum TensorTypeEnum {TENSOR_TYPE_UNKNOWN,TENSOR_TYPE_ROOT,TENSOR_TYPE_W,TENSOR_TYPE_U};

	enum IndexDirectionEnum {INDEX_DIR_IN, INDEX_DIR_OUT};

	enum IndexTypeEnum {INDEX_TYPE_SUMMED, INDEX_TYPE_FREE, INDEX_TYPE_DUMMY};

	TensorStanza(PsimagLite::String srep)
	    : conjugate_(false),
	      srep_(srep),
	      name_(""),
	      type_(TENSOR_TYPE_UNKNOWN)
	{
		VectorStringType tokens;
		PsimagLite::tokenizer(srep_,tokens,"|");
		SizeType ts = tokens.size();
		if (ts == 0 || ts > 2) {
			PsimagLite::String str("TensorStanza: malformed stanza ");
			throw PsimagLite::RuntimeError(str + srep_ + "\n");
		}

		PsimagLite::String nameAndId = getNameFromToken(tokens[0]);
		cleanToken(tokens[0]);
		setArgVector(insSi_,tokens[0]);

		if (ts == 2) {
			cleanToken(tokens[1]);
			setArgVector(outsSi_,tokens[1]);
		}

		SizeType ins = insSi_.size();
		SizeType outs = outsSi_.size();

		if (ins + outs == 0) {
			PsimagLite::String str("TensorStanza: no ins or outs ");
			throw PsimagLite::RuntimeError(str + srep_ + "\n");
		}

		SizeType l = nameAndId.length();
		if (l == 0) {
			PsimagLite::String str("TensorStanza: malformed partial srep, empty name/id ");
			throw PsimagLite::RuntimeError(str + srep_ + "\n");
		}

		SizeType tmp = std::count(nameAndId.begin(),nameAndId.end(),'*');
		if (tmp > 1) {
			PsimagLite::String str("TensorStanza: too many * in stanza ");
			throw PsimagLite::RuntimeError(str + srep_ + "\n");
		}

		conjugate_ = (tmp == 1);

		std::size_t index = nameAndId.find("*");
		if (index != PsimagLite::String::npos) nameAndId.erase(index,1);

		l = nameAndId.length();
		assert(l > 0);
		index = nameAndId.find_first_of("0123456789");

		name_ = nameAndId.substr(0,index);

		if (index == PsimagLite::String::npos)
			id_ = 0;
		else
			id_ = atoi(nameAndId.substr(index,l-index).c_str());

		if (name_ == "u") type_ = TENSOR_TYPE_U;
		if (name_ == "w") type_ = TENSOR_TYPE_W;
		if (name_ == "r") type_ = TENSOR_TYPE_ROOT;

		if (type_ != TENSOR_TYPE_ROOT && index == PsimagLite::String::npos) {
			PsimagLite::String str("TensorStanza: no digit for token ");
			throw PsimagLite::RuntimeError(str + nameAndId + "\n");
		}

		if (type_ == TENSOR_TYPE_UNKNOWN) {
			PsimagLite::String str("TensorStanza: partial srep, tensor type");
			throw PsimagLite::RuntimeError(str + srep_ + "\n");
		}
	}

	void conjugate()
	{
		conjugate_ = (!conjugate_);
		srep_ = srepFromObject();
	}

	bool isConjugate() const { return conjugate_; }

	const PsimagLite::String& sRep() const { return srep_; }

	SizeType id() const { return id_; }

	SizeType ins() const { return insSi_.size(); }

	SizeType outs() const { return outsSi_.size(); }

	IndexTypeEnum legType(SizeType ind, IndexDirectionEnum dir) const
	{
		char c = (dir == INDEX_DIR_IN) ? insSi_[ind].first : outsSi_[ind].first;
		if (c == 's') return INDEX_TYPE_SUMMED;
		if (c == 'f') return INDEX_TYPE_FREE;
		return INDEX_TYPE_DUMMY;
	}

	SizeType legTag(SizeType ind, IndexDirectionEnum dir) const
	{
		return (dir == INDEX_DIR_IN) ? insSi_[ind].second : outsSi_[ind].second;
	}

	TensorTypeEnum type() const { return type_; }

	friend std::ostream& operator<<(std::ostream& os, const TensorStanza& ts)
	{
		os<<"id= "<<ts.id_<<"\n";
		os<<"conjugate= "<<ts.conjugate_<<"\n";
		os<<"stanza= "<<ts.srep_<<"\n";
		os<<"name= "<<ts.name_<<"\n";
		os<<"type= "<<ts.type_<<"\n";
		os<<"needs ins and outs FIXME\n";
		//os<<"ins="<<ts.insSi_<<"\n";
		//os<<"outs="<<ts.outsSi_;<<"\n";
		return os;
	}

private:

	PsimagLite::String srepFromObject() const
	{
		PsimagLite::String srep = name_;
		if (conjugate_) srep += "*";
		srep += ttos(id_);
		srep += "(";
		for (SizeType i = 0; i < insSi_.size(); ++i) {
			srep += insSi_[i].first + ttos(insSi_[i].second);
			if (i == insSi_.size() - 1) continue;
			srep += ",";
		}

		if (outsSi_.size() == 0) {
			srep += ")";
			return srep;
		}

		srep += "|";
		for (SizeType i = 0; i < outsSi_.size(); ++i) {
			srep += outsSi_[i].first + ttos(outsSi_[i].second);
			if (i == outsSi_.size() - 1) continue;
			srep += ",";
		}

		srep += ")";
		return srep;
	}

	void setArgVector(VectorPairCharSizeType& si, PsimagLite::String part) const
	{
		if (part.length() == 0) return;
		VectorStringType tokens;
		PsimagLite::tokenizer(part,tokens,",");
		SizeType total = tokens.size();
		for (SizeType i = 0; i < total; ++i)
			si.push_back(getPairCharInt(tokens[i]));
	}

	PairCharSizeType getPairCharInt(PsimagLite::String token) const
	{
		SizeType l = token.length();

		if (l == 0) {
			PsimagLite::String str("TensorStanza: malformed stanza, ");
			throw PsimagLite::RuntimeError(str + token + "\n");
		}

		if (l == 1 && token != "d") {
			PsimagLite::String str("TensorStanza: malformed stanza, expecting a dummy, got ");
			throw PsimagLite::RuntimeError(str + token + "\n");
		}

		PsimagLite::String tmp = (l == 1) ? "0" : token.substr(1,l-1);
		return PairCharSizeType(token[0],atoi(tmp.c_str()));
	}

	PsimagLite::String getNameFromToken(PsimagLite::String token) const
	{
		std::size_t index = token.find("(");
		return token.substr(0,index);
	}

	void cleanToken(PsimagLite::String& token) const
	{
		SizeType l = token.length();
		assert(l > 0);
		std::size_t index = token.find("(");
		PsimagLite::String tmp = token;
		if (index != PsimagLite::String::npos)
			tmp = token.substr(index+1,token.length());

		if (token[l-1] == ')') tmp.substr(0,l-1);

		token = tmp;
	}

	SizeType id_;
	bool conjugate_;
	PsimagLite::String srep_;
	PsimagLite::String name_;
	TensorTypeEnum type_;
	VectorPairCharSizeType insSi_;
	VectorPairCharSizeType outsSi_;
};

} // namespace Mera
#endif // TENSORSTANZA_H
