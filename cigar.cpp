#include "cigar.h"

using namespace std;
//using namespace vcf;

void CigarElement::clear(void) {
    length = 0;
    type = '\0';
}

bool CigarElement::isInsertion(void) {
    return type == 'I';
}

bool CigarElement::isDeletion(void) {
    return type == 'D';
}

bool CigarElement::isSoftclip(void) {
    return type == 'S';
}

bool CigarElement::isIndel(void) {
    return isInsertion() || isDeletion();
}

std::ostream& operator<<(std::ostream& o, const CigarElement& e) {
    o << e.length << e.type;
    return o;
}

std::ostream& operator<<(std::ostream& o, const Cigar& c) {
    for (Cigar::const_iterator i = c.begin(); i != c.end(); ++i) {
        o << *i;
    }
    return o;
}

int Cigar::refLen(void) {
    int len = 0;
    for (Cigar::const_iterator c = begin(); c != end(); ++c) {
        if (c->type == 'M' || c->type == 'D' || c->type == 'X') {
            len += c->length;
        }
    }
    return len;
}

int Cigar::readLen(void) {
    int len = 0;
    for (Cigar::const_iterator c = begin(); c != end(); ++c) {
        if (c->type == 'M' || c->type == 'I' || c->type == 'X' || c->type == 'S') {
            len += c->length;
        }
    }
    return len;
}

int Cigar::softClipStart(void) {
    if (front().type == 'S') {
        return front().length;
    } else {
        return 0;
    }
}

int Cigar::softClipEnd(void) {
    if (back().type == 'S') {
        return back().length;
    } else {
        return 0;
    }
}

bool Cigar::isReference(void) {
    if (size() == 1 && front().type == 'M') {
        return true;
    } else {
        return false;
    }
}

void Cigar::append(const Cigar& c) {
    Cigar::const_iterator i = c.begin();
    // if the cigar is not length 0,
    // check end, if equivalent to this end, extend
    if (c.size() == 1 && c.front().length == 0) {
        // do nothing
    } else if (!empty()) {
        while (i != c.end() && back().type == i->type) {
            back().length += i->length;
            ++i;
        }
        while (i != c.end()) {
            push_back(*i);
            ++i;
        }
    } else {
        *this = c;
    }
}

string Cigar::str(void) {
    stringstream ss;
    ss << *this;
    return ss.str();
}

Cigar::Cigar(const string& cigarStr) {
    string number;
    string type;
    // strings go [Number][Type] ...
    for (string::const_iterator s = cigarStr.begin(); s != cigarStr.end(); ++s) {
        char c = *s;
        if (isdigit(c)) {
            if (type.empty()) {
                number += c;
            } else {
                // signal for next token, push back the last pair, clean up
                push_back(CigarElement(atoi(number.c_str()), type[0]));
                number.clear();
                type.clear();
                number += c;
            }
        } else {
            type += c;
        }
    }
    if (!number.empty() && !type.empty()) {
        push_back(CigarElement(atoi(number.c_str()), type[0]));
    }
}

Cigar::Cigar(int i, char t) {
    push_back(CigarElement(i, t));
}

Cigar::Cigar(vcf::VariantAllele& va) {
    char type = '\0';
    int length = 0;
    if (va.ref != va.alt) {
        if (va.ref.size() == va.alt.size()) {
            push_back(CigarElement(va.ref.size(), 'M'));
        } else if (va.ref.size() > va.alt.size()) {
            push_back(CigarElement(va.ref.size() - va.alt.size(), 'D'));
        } else {
            push_back(CigarElement(va.alt.size() - va.ref.size(), 'I'));
        }
    } else {
        push_back(CigarElement(va.ref.size(), 'M'));
    }
}

Cigar::Cigar(vector<vcf::VariantAllele>& vav) {
    char type = '\0';
    int length = 0;
    for (vector<vcf::VariantAllele>::iterator v = vav.begin(); v != vav.end(); ++v) {
        vcf::VariantAllele& va = *v;
        if (va.ref != va.alt) {
            if (type == 'M') {
                push_back(CigarElement(length, type));
                length = 0; type = '\0';
            }
            if (va.ref.size() == va.alt.size()) {
                push_back(CigarElement(va.ref.size(), 'M'));
            } else if (va.ref.size() > va.alt.size()) {
                push_back(CigarElement(va.ref.size() - va.alt.size(), 'D'));
            } else {
                push_back(CigarElement(va.alt.size() - va.ref.size(), 'I'));
            }
        } else {
            if (type == 'M') {
                length += va.ref.size();
            } else {
                length = va.ref.size();
                type = 'M';
            }
        }
    }
    if (type == 'M') {
        push_back(CigarElement(length, type));
    }
}

Cigar::Cigar(vector<BamTools::CigarOp>& cigarData) {
    for (vector<BamTools::CigarOp>::iterator o = cigarData.begin(); o != cigarData.end(); ++o) {
        push_back(CigarElement(o->Length, o->Type));
    }
}

Cigar::Cigar(gssw_cigar* c) {
    Cigar cigar;
    gssw_cigar_element* e = c->elements;
    for (int i = 0; i < c->length; ++i, ++e) {
        push_back(CigarElement(e->length, e->type));
    }
}

Cigar join(vector<Cigar>& cigars) {
    Cigar result;
    for (vector<Cigar>::iterator c = cigars.begin(); c != cigars.end(); ++c) {
        result.append(*c);
    }
    return result;
}

void Cigar::toCigarData(vector<BamTools::CigarOp>& cigarData) {
    cigarData.clear();
    for (Cigar::iterator c = begin(); c != end(); ++c) {
        BamTools::CigarOp op;
        op.Type = c->type;
        op.Length = c->length;
        cigarData.push_back(op);
    }
}
