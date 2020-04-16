#include<stdint.h>
#include<string>
#include<vector>
#include<algorithm>
#include<iostream>

#include "allele_functions.h"

void print_allele_depths(std::vector<allele> ad){
  std::cout << "AD Size: " << ad.size() << " " << std::endl;
  for(std::vector<allele>::iterator it = ad.begin(); it != ad.end(); ++it) {
    std::cout << "Nuc: " << it->nuc << std::endl;;
    std::cout << "Depth: " << it->depth << std::endl;
      std::cout << "Reverse: " << it->reverse <<std::endl;
    std::cout << "Qual: " << (uint16_t) it->mean_qual << std::endl;
    std::cout << "Beg: " << it->beg << std::endl;
    std::cout << "End: " << it->end << std::endl;
    std::cout << "Deleted bases: " << it->deleted_bases << std::endl << std::endl;
  }
}

int check_allele_exists(std::string n, std::string deleted_bases, std::vector<allele> ad){
  for(std::vector<allele>::iterator it = ad.begin(); it != ad.end(); ++it) {
    if(it->nuc.compare(n) == 0 && it->deleted_bases.compare(deleted_bases) == 0){
      return it - ad.begin();
    }
  }
  return -1;
}

bool is_indel(allele a){
  if(a.nuc.length() > 1 || !a.deleted_bases.empty())
    return true;
  return false;
}

bool is_del(allele a){
  if(!a.deleted_bases.empty())
    return true;
  return false;
}

int find_ref_in_allele(std::vector<allele> ad, char ref){ // For only SNVs
  std::string ref_s(1, ref);
  std::vector<allele>::iterator it = ad.begin();
  while(it < ad.end()){
    if(it->nuc.compare(ref_s) == 0 && it->deleted_bases.empty())
      return (it - ad.begin());
    it++;
  }
  return -1;
}



int find_ref_in_allele(std::vector<allele> ad, std::string deleted_bases, std::string ref){ // For indels
  std::vector<allele>::iterator it = ad.begin();
  while(it < ad.end()){
    if(it->nuc.compare(ref) == 0 && it->deleted_bases.compare(deleted_bases) == 0)
      return (it - ad.begin());
    it++;
  }
  return -1;
}

std::vector<allele> update_allele_depth(char ref,std::string bases, std::string qualities, uint8_t min_qual){
  std::vector<allele> ad;
  std::string indel;
  uint32_t i = 0, n =0, j = 0, q_ind = 0;
  bool beg, end;
  uint8_t q;
  std::string deleted_bases;
  std::string b;
  bool forward;
  while (i < bases.length()){
    b.clear();
    forward = true;
    beg = false;
    end = false;
    deleted_bases.clear();
    if(bases[i] == '^'){
      i += 2;			// Skip mapping quality as well (i+1) - 33
      continue;
    }
    if(bases[i] == '$'){
      i++;
      continue;
    }
    q = qualities[q_ind] - 33;
    allele tmp;
    switch(bases[i]){
    case '.':
      b = ref;
      end = (bases[i+1] == '$') ? (i+1 < bases.length()) : false;
      beg = (bases[i+1] == '^') ? (i+1 < bases.length()) : false;
      break;
    case ',':
      b = ref;
      forward = false;
      end = (bases[i+1] == '$') ? (i+1 < bases.length()) : false;
      beg = (bases[i+1] == '^') ? (i+1 < bases.length()) : false;
      break;
    case '#':			// For --reverse-del option
      b = bases[i];
      forward = false;
      break;
    case '*':
      b = bases[i];
      q = min_qual;		// For padding just add min_qual. mpileup by default seems to replicate BQs from next position
      break;
    case '+': case '-':		// Indels handled after
      break;
    default:
      int asc_val = bases[i];
      if(asc_val >= 65 && asc_val <= 90){
	b = bases[i];
      } else if(bases[i]>=97 && bases[i]<=122){
	b = bases[i] - ('a' - 'A');
	forward = false;
      }
      end = (bases[i+1] == '$');
      beg = (bases[i+1] == '^');
    }
    if(i+1 < bases.length() && (bases[i+1] == '+' || bases[i+1] == '-')){ // Indels
      j = i+2;
      while(isdigit(bases[j])){
	j++;
      }
      j = j - (i+2);
      n = stoi(bases.substr(i+2, j));
      indel = bases.substr(i+2+j, n);
      transform(indel.begin(), indel.end(), indel.begin(),::toupper);
      if(bases[i+1] == '+')	// For insertion remove + and add ref base
	b += indel;
      else {			// For deletion retain
	deleted_bases = indel;
      }
      i += n + j + 1;
      if(indel[0]>=97 && indel[0] <= 122)
	forward=false;
    }
    int ind = check_allele_exists(b, deleted_bases, ad);
    if(q >= min_qual){
      if (ind==-1){
	tmp.nuc = b;
	tmp.depth = 1;
	tmp.tmp_mean_qual = q;
	if(!forward)
	  tmp.reverse = 1;
	else
	  tmp.reverse = 0;
	if(beg)
	  tmp.beg = 1;
	else
	  tmp.beg = 0;
	if(end)
	  tmp.end = 1;
	else
	  tmp.end = 0;
	if(!deleted_bases.empty()) // Add deleted bases for deletions
	  tmp.deleted_bases = deleted_bases;
	else
	  tmp.deleted_bases = "";
	ad.push_back(tmp);
      } else {
	ad.at(ind).tmp_mean_qual = (ad.at(ind).tmp_mean_qual * ad.at(ind).depth + q)/(ad.at(ind).depth + 1);
	ad.at(ind).depth += 1;
	if(beg)
	  ad.at(ind).beg += 1;
	if(end)
	  ad.at(ind).end += 1;
	if(!forward)
	  ad.at(ind).reverse += 1;
      }
    }
    i++;
    q_ind++;
  }
  for(std::vector<allele>::iterator it = ad.begin(); it!=ad.end(); ++it){
    it->mean_qual = (uint8_t) it->tmp_mean_qual;
  }
  if(ad.size() > 0)
    std::sort(ad.begin(), ad.end());
  return ad;
}

int get_index(char a){
  switch(a){
  case'Y':
    a = 0;
    break;
  case'R':
    a = 1;
    break;
  case'W':
    a = 2;
    break;
  case'S':
    a = 3;
    break;
  case'K':
    a = 4;
    break;
  case'M':
    a = 5;
    break;
  case'A':
    a = 6;
    break;
  case'T':
    a = 7;
    break;
  case'G':
    a = 8;
    break;
  case'C':
    a = 9;
    break;
  case'D':
    a = 10;
    break;
  case'V':
    a = 11;
    break;
  case'H':
    a = 12;
    break;
  case'B':
    a = 13;
    break;
  default:
    a = -1;
  }
  return (int) a;
}

// Modified gt2iupac from bcftools.h. Expanded iupac matrix to include all ambigious nucleotides(added Y, R, W, S, K, M, A, T, G, C, D, V, H, B)  - https://github.com/samtools/bcftools/blob/b0376dff1ed70603c9490802f37883b9009215d2/bcftools.h#L48
/*
Expanded IUPAC matrix:

Y N H B B H H Y B Y N N H B
N R D V D V R D R V D V N N
H D W N D H W W D H D N H N
B V N S B V V B S S N V N B
B D D B K N D K K B D N N B
H V H V N M M H V M N V H N
H R W V D M A W R M D V H N
Y D W B K H W T K Y D N H B
B R D S K V R K G S D V N B
Y V H S B M M Y S C N V H B
N D D N D N D D D N D N N N
N V N V N V V N V V N V N N
H N H N N H H H N H N N H N
B N N B B N N B B B N N N B

 */
char gt2iupac(char a, char b){
  if(a == '*' || b == '*')
    return 'N';
  static const char iupac[14][14] = {
    {'Y', 'N', 'H', 'B', 'B', 'H', 'H', 'Y', 'B', 'Y', 'N', 'N', 'H', 'B'},
    {'N', 'R', 'D', 'V', 'D', 'V', 'R', 'D', 'R', 'V', 'D', 'V', 'N', 'N'},
    {'H', 'D', 'W', 'N', 'D', 'H', 'W', 'W', 'D', 'H', 'D', 'N', 'H', 'N'},
    {'B', 'V', 'N', 'S', 'B', 'V', 'V', 'B', 'S', 'S', 'N', 'V', 'N', 'B'},
    {'B', 'D', 'D', 'B', 'K', 'N', 'D', 'K', 'K', 'B', 'D', 'N', 'N', 'B'},
    {'H', 'V', 'H', 'V', 'N', 'M', 'M', 'H', 'V', 'M', 'N', 'V', 'H', 'N'},
    {'H', 'R', 'W', 'V', 'D', 'M', 'A', 'W', 'R', 'M', 'D', 'V', 'H', 'N'},
    {'Y', 'D', 'W', 'B', 'K', 'H', 'W', 'T', 'K', 'Y', 'D', 'N', 'H', 'B'},
    {'B', 'R', 'D', 'S', 'K', 'V', 'R', 'K', 'G', 'S', 'D', 'V', 'N', 'B'},
    {'Y', 'V', 'H', 'S', 'B', 'M', 'M', 'Y', 'S', 'C', 'N', 'V', 'H', 'B'},
    {'N', 'D', 'D', 'N', 'D', 'N', 'D', 'D', 'D', 'N', 'D', 'N', 'N', 'N'},
    {'N', 'V', 'N', 'V', 'N', 'V', 'V', 'N', 'V', 'V', 'N', 'V', 'N', 'N'},
    {'H', 'N', 'H', 'N', 'N', 'H', 'H', 'H', 'N', 'H', 'N', 'N', 'H', 'N'},
    {'B', 'N', 'N', 'B', 'B', 'N', 'N', 'B', 'B', 'B', 'N', 'N', 'N', 'B'}
  };
  if ( a>='a' ) a -= 'a' - 'A';
  if ( b>='a' ) b -= 'a' - 'A';
  int _a, _b;
  _a = get_index(a);
  _b = get_index(b);
  if(_a == -1 || _b == -1)
    return 'N';
  return iupac[_a][_b];
}

/*
  ATGC: 6,7,8,9 indices set up with get_index()
  Stop codon: *
  Unknown codon: X
  [[['AAA', 'AAT', 'AAG', 'AAC'],
    ['ATA', 'ATT', 'ATG', 'ATC'],
    ['AGA', 'AGT', 'AGG', 'AGC'],
    ['ACA', 'ACT', 'ACG', 'ACC']],
   [['TAA', 'TAT', 'TAG', 'TAC'],
    ['TTA', 'TTT', 'TTG', 'TTC'],
    ['TGA', 'TGT', 'TGG', 'TGC'],
    ['TCA', 'TCT', 'TCG', 'TCC']],
   [['GAA', 'GAT', 'GAG', 'GAC'],
    ['GTA', 'GTT', 'GTG', 'GTC'],
    ['GGA', 'GGT', 'GGG', 'GGC'],
    ['GCA', 'GCT', 'GCG', 'GCC']],
   [['CAA', 'CAT', 'CAG', 'CAC'],
    ['CTA', 'CTT', 'CTG', 'CTC'],
    ['CGA', 'CGT', 'CGG', 'CGC'],
    ['CCA', 'CCT', 'CCG', 'CCC']]]
 */

char codon2aa(char n1, char n2, char n3){
  if ( n1>='a' ) n1 -= 'a' - 'A';
  if ( n2>='a' ) n2 -= 'a' - 'A';
  if ( n3>='a' ) n3 -= 'a' - 'A';
  int _n1 = get_index(n1),
    _n2 = get_index(n2),
    _n3 = get_index(n3);
  static const char iupac_aa[4][4][4] = {
    {
      {'K', 'N', 'K', 'N'},	// 'AAA', 'AAT', 'AAG', 'AAC'
      {'I', 'I', 'M', 'I'},	// 'ATA', 'ATT', 'ATG', 'ATC'
      {'R', 'S', 'R', 'S'},	// 'AGA', 'AGT', 'AGG', 'AGC'
      {'T', 'T', 'T', 'T'}	// 'ACA', 'ACT', 'ACG', 'ACC'
    },
    {
      {'*', 'Y', '*', 'Y'},	// 'TAA', 'TAT', 'TAG', 'TAC'
      {'L', 'F', 'L', 'F'},	// 'TTA', 'TTT', 'TTG', 'TTC'
      {'*', 'C', 'W', 'C'},	// 'TGA', 'TGT', 'TGG', 'TGC'
      {'S', 'S', 'S', 'S'}	// 'TCA', 'TCT', 'TCG', 'TCC'
    },
    {
      {'E', 'D', 'E', 'D'},	// 'GAA', 'GAT', 'GAG', 'GAC'
      {'V', 'V', 'V', 'V'},	// 'GTA', 'GTT', 'GTG', 'GTC'
      {'G', 'G', 'G', 'G'},	// 'GGA', 'GGT', 'GGG', 'GGC'
      {'A', 'A', 'A', 'A'}	// 'GCA', 'GCT', 'GCG', 'GCC'
    },
    {
      {'Q', 'H', 'Q', 'H'},	// 'CAA', 'CAT', 'CAG', 'CAC'
      {'L', 'L', 'L', 'L'},	// 'CTA', 'CTT', 'CTG', 'CTC'
      {'R', 'R', 'R', 'R'},	// 'CGA', 'CGT', 'CGG', 'CGC'
      {'P', 'P', 'P', 'P'}	// 'CCA', 'CCT', 'CCG', 'CCC'
    }
  };
  if((_n1 < 6) || (_n1 > 9) || (_n2 < 6) || (_n2 > 9) || (_n3 < 6) || (_n3 > 9))
    return 'X';
  return iupac_aa[_n1-6][_n2-6][_n3-6];
}

void get_alleles_by_threshold(std::vector<allele> &aalt, double threshold, uint32_t total_depth){
  std::sort(aalt.begin(), aalt.end(), std::greater<allele>()); // Sort by depth in desc order
  std::vector<allele>::iterator it = aalt.begin();
  it = aalt.begin();
  double cur_threshold;
  uint32_t cur_depth;
  cur_depth = it->depth;
  cur_threshold = (double)cur_depth/(double)total_depth;
  it++;
  while(it != aalt.end() && (cur_threshold < threshold || it->depth == (it-1)->depth)){
    cur_depth += it->depth;
    cur_threshold = (double)cur_depth/(double)total_depth;
    it++;
  }
  if(it<aalt.end())
    aalt.erase(it, aalt.end());
}

uint32_t get_total_depth(std::vector<allele> vec){
  uint32_t d = 0;
  for (std::vector<allele>::iterator it = vec.begin(); it != vec.end(); ++it) {
    d += it->depth;
  }
  return d;
}

uint8_t get_longest_insertion(std::vector<allele> vec){
  std::vector<allele>::iterator res;
  res = std::max_element(vec.begin(), vec.end(), [](allele &a, allele &b) {
      return a.nuc.length() < b.nuc.length();
    });
  return res->nuc.length();
}
