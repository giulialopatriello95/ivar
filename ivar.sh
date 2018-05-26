#!/bin/bash

usage_dir="./usage"

usage() {
    case "$1" in
	trim)
	    echo `cat ${usage_dir}/usage_$1.txt` 1>&2; exit 1;
	    ;;
	removereads)
	    echo `cat ${usage_dir}/usage_$1.txt` 1>&2; exit 1;
	    ;;
	callvariants)
	    echo `cat ${usage_dir}/usage_$1.txt` 1>&2; exit 1;
	    ;;
	filtervariants)
	    echo `cat ${usage_dir}/usage_$1.txt` 1>&2; exit 1;
	    ;;
	consensus)
	    echo `cat ${usage_dir}/usage_$1.txt` 1>&2; exit 1;
	    ;;
	createbed)
	    echo `cat ${usage_dir}/usage_$1.txt` 1>&2; exit 1;
	    ;;
	getmasked)
	    echo `cat ${usage_dir}/usage_$1.txt` 1>&2; exit 1;
	    ;;
	*)
	    echo `cat ${usage_dir}/usage_all.txt` 1>&2; exit 1;
	    ;;
    esac
}

cmd=$1; shift
case "$cmd" in
    trim)
	while getopts ":i:p:b:" o; do
	    case "${o}" in
		i)
		    i=${OPTARG}
		    ;;
		b)
		    b=${OPTARG}
		    ;;
		p)
		    p=${OPTARG}
		    ;;
		*)
		    usage trim
		    ;;
	    esac
	done
	shift $((OPTIND-1))
	if [ -z "${i}" ] || [ -z "${p}" ] || [ -z "${b}" ]; then
	    usage trim
	fi
	~/Documents/code/ivar/trim_primer_quality ${i} ${b} ${p}.trimmed.bam
	echo "Sorting"
	samtools sort -o ${p}.trimmed.sorted.bam ${p}.trimmed.bam
	echo "Indexing"
	samtools index ${p}.trimmed.sorted.bam ${p}.trimmed.sorted.bai
	;;
    removereads)
	while getopts ":i:p:a:" o; do
	    case "${o}" in
		i)
		    i=${OPTARG}
		    ;;
		p)
		    p=${OPTARG}
		    ;;
		*)
		    usage removereads
		    ;;
	    esac
	done
	shift $((OPTIND-1))
	if [ -z "${i}" ] || [ -z "${p}" ]; then
	    usage removereads
	fi
	~/Documents/code/ivar/remove_reads_from_amplicon ${i} ${p}.filtered.bam "$@"
	echo "Sorting"
	samtools sort -o ${p}.filtered.sorted.bam ${p}.filtered.bam
	echo "Indexing"
	samtools index ${p}.filtered.sorted.bam ${p}.filtered.sorted.bai
	;;
    callvariants)
	while getopts ":i:p:r:" o; do
	    case "${o}" in
		i)
		    i=${OPTARG}
		    ;;
		p)
		    p=${OPTARG}
		    ;;
		r)
		    r=${OPTARG}
		    ;;
		*)
		    usage callvariants
		    ;;
	    esac
	done
	shift $((OPTIND-1))
	if [ -z "${i}" ] || [ -z "${p}" ] || [ -z "${r}" ]; then
	    usage callvariants
	fi
	samtools mpileup -A -B -Q 0 -d 300000 -gu -t AD -pm 1 -F 0 -f ${r} ${i} | bcftools call --ploidy 1 -m -A -Oz -o ${p}.vcf.gz
	bcftools index ${p}.vcf.gz
	;;
    filtervariants)
	while getopts ":p:f:b:" o; do
	    case "${o}" in
		p)
		    p=${OPTARG}
		    ;;
		f)
		    f=${OPTARG}
		    ;;
		b)
		    b=${OPTARG}
		    ;;
		*)
		    usage filtervariants
		    ;;
	    esac
	done
	shift $((OPTIND-1))
	if [ -z "${p}" ] || [ -z "${f}" ] || [ -z "${b}" ]; then
	    usage filtervariants
	fi
	~/Documents/code/ivar/combine_variants.py ${p} ${f} ${b} "$@"
	;;
    getmasked)
	while getopts ":p:f:b:" o; do
	    case "${o}" in
		p)
		    p=${OPTARG}
		    ;;
		f)
		    f=${OPTARG}
		    ;;
		b)
		    b=${OPTARG}
		    ;;
		*)
		    usage getmasked
		    ;;
	    esac
	done
	shift $((OPTIND-1))
	if [ -z "${p}" ] || [ -z "${f}" ] || [ -z "${b}" ]; then
	    usage getmasked
	fi
	~/Documents/code/ivar/get_masked_amplicons.py ${p} ${f} ${b} "$@"
	;;
    consensus)
	while getopts ":i:p:r:" o; do
	    case "${o}" in
		i)
		    i=${OPTARG}
		    ;;
		p)
		    p=${OPTARG}
		    ;;
		r)
		    r=${OPTARG}
		    ;;
		*)
		    usage consensus
		    ;;
	    esac
	done
	shift $((OPTIND-1))
	if [ -z "${i}" ] || [ -z "${r}" ] || [ -z "${p}" ]; then
	    usage consensus
	fi
	cat ${r} | bcftools consensus $i > ${p}.fa
	;;
    createbed)
	while getopts ":c:p:r:" o; do
	    case "${o}" in
		c)
		    c=${OPTARG}
		    ;;
		p)
		    p=${OPTARG}
		    ;;
		r)
		    r=${OPTARG}
		    ;;
		*)
		    usage createbed
		    ;;
	    esac
	done
	shift $((OPTIND-1))
	if [ -z "${i}" ] || [ -z "${r}" ] || [ -z "${p}" ]; then
	    usage createbed
	fi
	~/Documents/code/ivar/primer_bam_to_bed.sh $c $p $r
	;;
    *)
	usage
esac
