<?php

/**
 * Defines the second step in the importer form.
 * which is to map the csv columns to corresponding fields/properties
 * 
 * Requires a helper function which build the options to map to.
 */
function importer_confirm_data($form, &$form_state) {
  $step_data = $form_state['step_information'];

  // unify all the collected information.
  $info = array();

  $info['file_data'] = $step_data[1]['stored_values'];
  $info['identifier_field_name'] = $step_data[2]['stored_values']['identifier_field_name'];
  $info['csv_map'] = $step_data[2]['stored_values']['csv_map'];

  // build the table
  $table = array(
  	'attributes' => array(),
  	'caption' => 'confirmation',
  	'colgroups' => array(),
  	'sticky' => NULL,
  	'empty' => 'Empty set',
  );
  $table['header'] = array('CSV column','Maps to','Single / Multiple', 'Delimiter', 'Is identifier');
  $table['rows'] = array();

  foreach($info['csv_map'] AS $col => $data) {
  	$sing_mult = $data['is_multiple'] ? 'Multiple' : 'Single';

  	$is_identifier = ($data['maps_to'] == $info['identifier_field_name']) ? 'YES' : 'NO';

  	$row = array($col, $data['maps_to'], $sing_mult, $data['subfield_delimiter'], $is_identifier);

  	array_push($table['rows'], $row);
  }

  $table_element = theme_table($table);


  // build the form
  $form = array();

  $form['confirm_data'] = array(
  	'#type' => 'checkbox',
  	'#title' => 'Confirm import data checkbox',
  	'#description' => 'Be extremely careful! This action cannot be undone.',
  	'#prefix' => $table_element,

  	'#required' => TRUE,

  	'#default_value' => 0,
  );

  return $form;
};