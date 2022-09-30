<?php

include_once('../search.protect.inc');		// trivial access check - include it first!

?><!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">
<?php
$dirBase = '../../web/';

if ( $html = @file_get_contents($dirBase.'search/search.header.html') ) { echo $html; }


echo '<body>';

if ( $html = @file_get_contents($dirBase.'search/search.navigation.html') ) { echo $html; }

echo '<div style="width:80%; margin-left:10%; margin-top:2.5em; margin-bottom:2.5em;">';
if ( $html = @file_get_contents($dirBase.'search/drupal.node-code.html') ) { echo $html; }
echo '</div>';

echo '</body>';

?>
</html>

