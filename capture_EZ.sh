#!/bin/bash

bold=$(tput bold)
red=$(tput setaf 1)
green=$(tput setaf 2)
normal=$(tput sgr0)

modulo10() {
   # takes a string of digits as input
   # returns 0 if checksum is correct as per Swiss BESR requirements
   #         1-9 if incorrect
   #         negative number if error
   local -a table=( 0 9 4 6 8 2 7 1 3 5 );
   local next=0;
   local nummer=$1
   for (( i=0; i<${#nummer}; i++ )); do
#   for (( i=0; i<5; i++ )); do
      local ziffer=${nummer:$i:1}
      local m=$(( (next + ziffer) % 10 ))
      next=${table[m]}
   done
   # next=$(( (10 - next) % 10 ))
   echo $next
   # return $next
}

#sleep 5
while (true); do
   imagefile=$( mktemp --suff=.jpg )
   # cpen_get_image $imagefile
   # numberstring=$( tesseract "$imagefile" stdout -psm 7 -c tessedit_char_whitelist="0123456789+>" )
   rm "$imagefile"
   numberstring='0100003949753>120000000000234478943216899+ 010001628>'
   echo -e "\n${bold}${green}${numberstring}${normal}"
   correct=0
   numberstring=$( echo "$numberstring" | sed 's/ //g' )
   declare recordtype='unknown'
   if [ "${numberstring:0:2}" == "01" ] ; then
      if [ "${numberstring:13:1}" == ">" ] ; then recordtype='CHF'; else echo "'${numberstring:13:1}' should be '>'" >&2; fi
   elif [ "${numberstring:0:2}" == "04" ] ; then
      if [ "${numberstring:3:1}" == ">" ] ; then recordtype='not specified'; fi
   else
      echo "Unknown record type of '$numberstring'"  >&2
      correct=1
   fi
   declare -a pieces
   if [ $correct -eq 0 ] ; then
      echo "recordtype is $recordtype; now split '$numberstring' into pieces" >&2
      numberstring=$( echo "$numberstring" | sed 's/>/+/g' )
      IFS=+ read -r amount reference account <<<"$numberstring"
      echo "Amount = $amount, Reference = $reference, Account = $account" >&2
      pieces=( $amount $reference $account )
      for piece1 in "${pieces[@]}"; do
         checksum=$( modulo10 $piece1 )
         echo "checksum for '$piece1': '$checksum'" >&2
         if [ $checksum -ne 0 ] ; then correct=1; fi
      done
   fi
   if [ $correct -eq 0 ] ; then
      xdotool search --classname 'navigator' windowactivate
      echo "type results to window '$windowid'" >&2
      amount=${amount:2} # remove record type identifier
      amount="$( echo "$amount" | sed -e 's/^0*//' )"  # remove leading zeros
      amount=${amount:0:-1}  # remove checksum
      if [ -n "$amount" ] ; then amount="${amount:0:-2}.${amount: -2}" ; fi
      echo "amount: ${green}$amount${normal} $recordtype"
      echo "account: ${green}${account:0:2}-$( echo "${account:2:-1}" | sed 's/^0*//' )-${account: -1}${normal}"
      echo "reference: ${green}$reference${normal}"
      xdotool type "${account:0:2}"
      xdotool key minus type $( echo "${account:2:-1}" | sed 's/^0*//' )
      xdotool key minus type "${account: -1}"
      xdotool key Tab type "$amount"
      xdotool key Tab type "$reference"
   fi
   exit 0
done
