<?php

/**
 * Defines the second step in the importer form.
 * which is to map the csv columns to corresponding fields/properties
 * 
 * Requires a helper function which build the options to map to.
 */
function importer_map_fields($form, &$form_state) {
  $prev_step = (int) $form_state['step'] - 1;

  $stored = $form_state['step_information'][$prev_step]['stored_values'];

  $delimiter = isset($stored['csv_delimiter']) ? $stored['csv_delimiter'] : ',';
  $enclosure = isset($stored['csv_enclosure']) ? $stored['csv_enclosure'] : '"';

  // get the first line of the CSV file
  $file_obj = file_load($stored['file']);
  $file_resource = fopen($file_obj->uri, 'r');
  $csv_first_line = fgetcsv($file_resource, NULL, $delimiter, $enclosure);

  // get the column names
  if ($stored['csv_has_column_names']) {
    // if file has the column names in the first row..
    $csv_col_names = $csv_first_line;
  } else {
    // set the column names as numbers.
    $csv_col_names = array_keys($csv_first_line);
  }

  // get the options for the column correspondant.
  $map_options = importer_helper_get_field_and_props($stored['entity_type'], $stored['entity_subtype']);

  // start the form building process.
  $form = array();


  $form['csv_col_names'] = array(
    '#type' => 'hidden',
    '#title' => t('This field holds an array keyed by the csv_col_no, with the csv_has_column_names'),
    '#value' => $csv_col_names,
  );


  $form['csv_map'] = array(
    '#type' => 'fieldset', 
    '#title' => t('Field Mapping'),
    '#description' => t('Map your .CSV columns to the node Fields'),
    '#collapsible' => TRUE, 
    '#collapsed' => FALSE,

    '#tree' => TRUE,
  );


  // set the columns as form fields.
  // we are building the "field mapper"
  foreach($csv_col_names AS $column_no => $column_name) {
    // calll helper function.
    $suggested_field = importer_helper_get_suggested_correspondant($column_name, $map_options);


    // THE MAP IS KEYED BY THE COLUMN NUMBERS
    $form['csv_map'][$column_name] = array(
      '#type' => 'fieldset',
      '#title' => t($column_name),
      '#collapsible' => TRUE, 
      '#collapsed' => FALSE,

      '#tree' => TRUE,
    );

    // save the csv column name as a property
    $form['csv_map'][$column_name]['column_no'] = array(
      '#type' => 'hidden',
      '#value' => $column_no,
      '#title' => t($column_name . ' col number is '. $column_no),
    );

    $form['csv_map'][$column_name]['maps_to'] = array(
      '#type' => 'select',
      '#options' => $map_options,

      // '#required' => TRUE,     // we comment out '#required' for testing purposes
      '#empty_option' => '- Not mapped -',

      '#title' => t($column_name),
      '#default_value' => !empty($form_state['values'][$column_name]) ? $form_state['values'][$column_name] : $suggested_field,
    );

    $form['csv_map'][$column_name]['default_value'] = array(
      '#type' => 'textfield',
      '#title' => 'Field default value.',
      '#description' => 'If the CSV column is empty, what should be it\'s value?',
    );

    $form['csv_map'][$column_name]['is_multiple'] = array(
      '#type' => 'checkbox',
      '#title' => t($column_name . ' is a multi-value field.'),

      '#default_value' => 0,
    );

    $form['csv_map'][$column_name]['subfield_delimiter'] = array(
      '#type' => 'textfield',
      '#title' => t('Subfield delimiter'),
      '#size' => 1,

      '#default_value' => ',',
    );

  }




  // select a identifier field.
  $form['identifier_field_name'] = array(
    '#type' => 'select',
    '#title' => t('Select the ID field for the entity'),
    '#description' => t('This field will be used to check for updating entities. It should be a completely unique field for the entity. For example, for \'nodes\', the ID is \'nid\' field.'),

    '#options' => $map_options,
    '#empty_option' => '- Select -',
  );


  return $form;
}

/////////////////////////////////
/////////// HELPERS /////////////
/////////////////////////////////

// returns a list of properties AND fields, keyed by the machine names and with label names as values
function importer_helper_get_field_and_props($entity, $bundle) {

  // now that we know the columns to be mapped FROM,
  // we need to get the fields and properties to map TO. That depends on the node type..

	// get the fields.
  $fields = field_info_instances($entity, $bundle);

  // get the properties
  $properties_info = entity_get_property_info($entity);
  $properties = array_keys( $properties_info['properties'] );

  // this function is used to build the options for the fields select
  function get_field_label($field_instance) {
    return $field_instance['label'];
  }

  // store field options
  $field_options = (array) array_map('get_field_label', $fields);
  $property_options = array_combine($properties, $properties);


  // build the options by merging both property options and field options.
  $map_options = array_merge($field_options, $property_options);

  return $map_options;
}


// searches through the map options for the column name and returns a suggested correspondant.
function importer_helper_get_suggested_correspondant($column_name, $map_options) {

    // THIS IS AWESOMELY CONFUSING..
    // try to find the column name inside the fields of the node
    // first try to match the column_name with the machine_names
    // search within keys
    $machine_name_match = array_key_exists($column_name, $map_options);
    // then try to match the column_name with the Labels
    // search within values
    $label_name_match = array_search($column_name, $map_options);

    if ($machine_name_match) {
      $suggested_field = $column_name;
    } else if ($label_name_match) {
      $suggested_field = $label_name_match;
    } else {
      $suggested_field = NULL;
    }

    return $suggested_field;
}


