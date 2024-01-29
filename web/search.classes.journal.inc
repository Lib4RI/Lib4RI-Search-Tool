<?php

@include_once('./search.config.inc');


// ==================================================================================

class apiQueryUtilsIssn extends apiQueryUtils
{
	// remind, $searchTerm may (also) be the (e)ISSN
	public $issnAry;	// ISSN(s) of the journal we are insterested in
	public $pIssn;		// for (print) issn *found*
	public $eIssn;		// for electronical issn *found*

	public function __construct($apiLabel)
	{
		parent::__construct($apiLabel);
		$this->setupParam();
		$this->setupIssn();
	}
	
	public function setupIssn($issnList = '')
	{
		if ( ( $tmp = trim($issnList) ) || ( $tmp = @trim(strip_tags($_GET['issn'])) ) ) {
			$this->issnAry = array_map('trim',explode(',',strtr($tmp,';',',')));
			return;
		}
		$issnList = $this->cleanIssn($this->searchTerm);	// assumption
		if ( @strpos($issnList,'-') ) {		// rough check, so not empty
			$ary = array();
			preg_match_all('/\d{4}\s*\-\s*\d{4}|\d{4}\s*\-\s*\d{3}X/',$issnList,$ary);
			$this->issnAry = array_map( array($this,'cleanIssn'), array_values($ary[0]) );
		}
	}

	/*
		Will turn an array with CC licenses into a sting/list where 'CC BY' will get at LibRI recommendation hint.
		So for CC BY we add 'recommended', for example:
			intput: [ 'CC BY 3.0' , 'CC BY-ND' ]
			output: "CC BY 3.0 (recomended!) or CC BY-ND"
		Also multiple redundant values will be filtered. No lower/upper case changes so far.
	*/
	public function addLib4riLicReco($licAry = null, $licSep = '')	/* will turn array with CC licenses into a term with LibRI recommendation */
	{
		if ( is_array($licAry) ) {
			$licAry = array_map('strip_tags',$licAry);
		} else { /* let's assume a string */
			$licAry = explode( ',', preg_replace('/\s+/',' ',str_replace([' or ',' / '],[',',','],strip_tags($licAry))) );
		}
		$licAry = array_filter( array_map('trim',$licAry) );
		$htmlAry = [];
		foreach( $licAry as $lic ) { // similar as array_unique() would do, just case-INsensitive
			$htmlAry[strtoupper($lic)] = $lic;
		}
		// Tune even for chaotic input like "CC BY 3.0, CC BY (4.0) or CC BY-ND":
		foreach( $htmlAry as $idx => $lic ) {
			$htmlAry[$idx] = '<span style="white-space:nowrap;">' . $lic . '</span>';
			if ( sizeof($htmlAry) > 1 && substr($lic.' ',0,6) == 'CC BY ' ) { // adding 1 space just for processing help
				if ( strlen($lic) < 6 || substr($lic.' ',6,1) == '(' || intval(substr($lic.' ',6,1)) ) {
					$htmlAry[$idx] .= ' (<a href="https://www.lib4ri.ch/creative-commons-licences#which-licence" target="_blank" title="Recommended by Lib4RI">';
					$htmlAry[$idx] .= '<span class="lib4ri-recommended">recommended</span></a>)';
				}
			}
		}
		if ( !empty($licSep) || sizeof($htmlAry) < 2 ) {
			return implode( $licSep, $htmlAry );
		}
		return implode(', ', array_slice($htmlAry,0,-1) ) . ' or ' . array_pop($htmlAry);
	}

}

// ==================================================================================

// Including all inc-files with the (extended) API classes for available online-services we are going to contact.
// Convention: To properly load the extended classes we rely on the file name: the more dot-separated parts it has,
// the more extended the classes inside are assmed to be (resp. parent classes reside in files with less name parts).
$dirInc = './service/';
$typeInc = 'journal';
$classAry = [];
foreach( @scandir($dirInc) as $classItem ) {
	if ( substr($classItem,-4) != '.inc' ) { continue; }
	if ( substr($classItem,0,strlen($typeInc)+1) != $typeInc.'.' ) { continue; }
	if ( !filesize($dirInc.$classItem) ) { continue; }
	$classAry[count(explode('.',$classItem))][] = $classItem;
}
for($e=2;$e<8;$e++) {
	if ( @empty($classAry[$e]) ) { continue; }
	foreach( $classAry[$e] as $classItem ) { include_once($dirInc.$classItem); }
}

// -----------------------------------------------------------------------------------
