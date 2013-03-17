<?php


/**
 * 
 */
function importer_file_and_type($form, &$form_state) {

  $form = array();

  // The CSV file.

  $form['file'] = array(
    '#type' => 'managed_file',
    '#name' => 'csv_file',
    '#title' => t('CSV file'),
    '#default_value' => !empty($form_state['values']['file']) ? $form_state['values']['file'] : '',
    '#upload_validators' => array(
        'file_validate_extensions' => array('csv'),
        // Pass the maximum file size in bytes
//        'file_validate_size' => array(MAX_FILE_SIZE*1024*1024),
    ),
    '#required' => TRUE,
  );

  // CSV file config
  $form['config'] = array(
    '#type' => 'fieldset', 
    '#title' => t('File configurations'),
    '#weight' => 0,
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );

  // Metadata on the file
  $form['config']['csv_has_column_names'] = array(
    '#type' => 'checkbox',
    '#name' => 'csv_has_column_names',
    '#title' => t('Does the .CSV file have the column names in the first row?'),
    '#default_value' => !empty($form_state['values']['csv_has_column_names']) ? $form_state['values']['csv_has_column_names'] : 1,
  );

  $form['config']['csv_delimiter'] = array(
    '#type' => 'textfield',
    '#name' => 'csv_delimiter',
    '#title' => t('CSV field delimiter (stuff that separates fields)'),
    '#size' => 1,
    '#default_value' => !empty($form_state['values']['csv_delimiter']) ? $form_state['values']['csv_delimiter'] : ',',
  );

  $form['config']['csv_enclosure'] = array(
    '#type' => 'textfield',
    '#name' => 'csv_enclosure',
    '#title' => t('CSV field enclosure (stuff that wraps the field value)'),
    '#size' => 1,
    '#default_value' => !empty($form_state['values']['csv_enclosure']) ? $form_state['values']['csv_enclosure'] : '"',
  );



  $path_to_mod = drupal_get_path('module', 'importer');
  $path_to_default_temp = $path_to_mod . '/temp';

  $form['uploads'] = array(
    '#type' => 'fieldset',
    '#title' => t('Upload configurations'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );

  $form['uploads']['temp_file_base_path'] = array(
    '#type' => 'textfield',
    '#name' => 'temp_file_base_path',
    '#title' => t('The path to the directory where your to-be-uploaded-files are located.'),
    '#description' => t('The full path to your files should be path_to_dir/@file_name'),
    '#default_value' => !empty($form_state['values']['temp_file_base_path']) ? $form_state['values']['temp_file_base_path'] : $path_to_default_temp,
  );






//~??~?~?~?~?~?~~?~?~?~?~//
/// EXPAND THIS LATER ///

  // ENTITY INFO
  $entity_info = entity_get_info();

  // ENTITY INFO
  $entity_types = array_keys($entity_info);

  // ENTITY SUB-TYPES

/// EXPAND THIS LATER ///



  ////////////////////////////////////
  ///////// HARD CODING FOR NODE /////
  ////////////////////////////////////
  $form['entity_type'] = array(
    '#title' => 'Entity type to be imported.',
    '#type' => 'hidden',
    '#default_value' => 'node',
  );

  // NODE TYPE

  $node_types = node_type_get_types();

  function importer_get_node_type_name($node_type) {
    return $node_type->name;
  }



  $node_type_by_names = (array) array_map('importer_get_node_type_name', $node_types);
  $form['entity_subtype'] = array(
    '#type' => 'select',
    '#options' => $node_type_by_names,

    '#title' => t('Select the node type to import'),
    '#default_value' => !empty($form_state['values']['entity_subtype']) ? $form_state['values']['entity_subtype'] : '',
  );
  return $form;
}