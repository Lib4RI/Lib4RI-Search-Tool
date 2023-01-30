<?php

include_once('../search.protect.inc');		// trivial access check - include it first!

?><!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">
<?php
$dirBase = '../../web/';

if ( $html = @file_get_contents($dirBase.'search/search.header.html') ) { echo $html; }


echo '<body>';

echo '<style type="text/css"><!--' . "\r\n";
echo @file_get_contents($dirBase.'css/lib4ri-websearch.css');
echo "\r\n" . '--></style>' . "\r\n\r\n";

if ( $html = @file_get_contents($dirBase.'search/search.navigation.html') ) { echo $html; }

if ( $html = @file_get_contents($dirBase.'search/drupal.node-code.html') ) { echo $html; }

echo '</body>';

?>
</html>

