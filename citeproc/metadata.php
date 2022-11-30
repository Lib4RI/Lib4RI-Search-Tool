<?php
include "vendor/autoload.php";
use Seboettg\CiteProc\StyleSheet;
use Seboettg\CiteProc\CiteProc;

$data = file_get_contents("metadata.json");
$style = StyleSheet::loadStyleSheet("din-1505-2");
$citeProc = new CiteProc($style);
echo $citeProc->render(json_decode($data), "bibliography");


