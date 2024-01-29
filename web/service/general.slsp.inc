<?php

class apiQuerySLSP extends apiQueryUtils
{
	public function __construct($apiLabel)
	{
		parent::__construct($apiLabel);
		$this->setupParam();
	}

	public function getQueryField($searchTerm = '', $qField = '')
	{
		if ( !in_array(strtolower($qField),['issn','isbn']) ) {
			$ary = [];
			$searchTerm = strtr($searchTerm,'[{(;:._)}]','||||||||||');	// accepted word/number limiters
			$searchTerm = preg_replace('/[^\w|^\d]/','',$searchTerm); // to only have 0-9 or a-z, space-less
			preg_match('/^\d+/',$searchTerm,$ary);
			if ( @strlen($ary[0]) > 6 ) {
				if ( strlen($ary[0]) > 9 ) {
					return 'isbn'; // ISBN10 or ISBN13
				}
				if ( strlen($ary[0]) > 7 ) {
					return 'issn'; // ISSN like 0123-4567
				}
				if ( strtoupper(substr($searchTerm,7,1)) == 'X' ) {
					return 'issn'; // ISSN like 0123-789X
				}
			}
		}
		return ( empty($qField) ? 'any' : $qField );
	}
}
