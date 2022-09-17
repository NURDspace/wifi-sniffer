#! /bin/sh

if [ -e mac-vendor.txt ] ; then
	echo file exists
	exit 1
fi

rm -f oui.txt

wget https://standards-oui.ieee.org/oui/oui.txt

grep ^..-..-.. oui.txt | sed -e 's/ *	*(hex) *	*/ /g' > mac-vendor.txt
