<?php

/**
 * This file contains the logic that will loop through the CSV data.
 * This method is called twice in the importer form flow:
 * once for checking the creations and updates
 * and another time for effectively creating the nodes.
 */
function importer_loop_csv($form, &$form_state) {

	$step_data = $form_state['step_information'];


	///////////////////
	//// VARS /////////
	///////////////////

	// file info was set on the first form step.
	$file_info = $step_data[1]['stored_values'];

	// identifier field name was set on the second step
	$identifier_field_name = $step_data[2]['stored_values']['identifier_field_name'];
	$csv_col_names = $step_data[2]['stored_values']['csv_col_names'];
	$csv_map = $step_data[2]['stored_values']['csv_map'];

	// first load the csv file.
	// get the first line of the CSV file
	$delimiter = $file_info['csv_delimiter'];
	$enclosure = $file_info['csv_enclosure'];

	$file_drupal_obj = file_load($file_info['file']);
	$file_resource = fopen($file_drupal_obj->uri, 'r');

	// variables to hold the newly created resources
	// and also the updated ones
	$to_create_wrappers = array();
	$to_create_displayers = array();

	$to_update_wrappers = array();
	$to_update_displayers = array();

	///////////////////
	/////// VARS //////
	///////////////////




	// $row_no is used just to discard the first row in case 
	// the csv file contains the column names
	$row_no = 0;

	while( ($row_columns = fgetcsv($file_resource, NULL, $delimiter, $enclosure) ) !== FALSE ) {

/*
		drupal_set_message('---------------------');
		drupal_set_message('ROW '. $row_no);

		drupal_set_message('row column count: '. count($row_columns));
		drupal_set_message( t('<pre>@cols</pre>', array('@cols' => print_r($row_columns, TRUE))) );

		*/

		if ($row_no == 0 && $file_info['csv_has_column_names']) {
			// skip row 0 if it has the column names
			$row_no += 1;
			continue;
		}

		// $row_no is used just to discard the first row in case 
		// the csv file contains the column names	
		$row_no += 1;


		//////////////////////////////////
		///// FIRST STEP: GET ENTITY /////
		//////////////////////////////////

		// declare a var to hold the entity
		$entity = NULL;
		// declare var to hold the action to be done.
		$action = NULL;

		// first check if the entity already exists
		if ( isset($identifier_field) && $identifier_field != '' ) {
			$identifier_field_value = $row_columns[$identifier_field_name];

			// entity_load function returns an array of entities
			$entity = entity_load(
				$file_info['entity_type'], 			// the entity type
				array( $identifier_field_value ) 	// the identifier field value
			);

			$entity = $entity[ $identifier_field_value ];
		}

		// if the entity already exists
		if ( isset($entity) ) {
			// just update
			$action = 'update';

		} else {
			// create
			$action = 'create';

			// create a new entity instance, creating an empty data array for it.
			$entity_data = array();

			// if there is a type/bundle set, use it
			if ( isset($file_info['entity_subtype']) )  {
				$entity_data['type'] = $file_info['entity_subtype'];
			}

			$entity = entity_create( $file_info['entity_type'], $entity_data);
		}



		//////////////////////////////////////////
		///// SECOND STEP: BUILD THE WRAPPER /////
		//////////////////////////////////////////

		// ImporterEntityMetadataWrapper is defined below
		$importer_wrapper = new ImporterEntityMetadataWrapper($file_info['entity_type'], $entity );




		/////////////////////////////////////////////////////
		////// THIRD STEP: SET THE VALUES ON THE WRAPPER ////
		/////////////////////////////////////////////////////

		// loop through the row columns
		foreach ($row_columns as $col_no => $col_value) {

			// vars 
			// $csv_col_names,
			// were all defined in the begining, at VARS

			// get the col_name
			$csv_col_name = $csv_col_names[ $col_no ];

			// get the map info object
			$csv_column_info = $csv_map[ $csv_col_name ];

			// if the column is the identifier column, do not set it.
			if ($csv_column_info['maps_to'] == $identifier_field_name) {
				continue;
			}

			// the column's default value
			$col_default_value = $csv_column_info['default_value'];

			// only set on the importer_wrapper if
			// the csv_column was mapped to any field
			
			if ( isset($csv_column_info['maps_to']) && $csv_column_info['maps_to'] != NULL ) {

				// check if the column_value is a multiple value.
				if ($csv_column_info['is_multiple']) {

					$delimiter = $csv_column_info['subfield_delimiter'];

					$col_value = explode($delimiter, $col_value);
					$col_value = array_map('trim', $col_value);

					// also break the default value
					$col_default_value = explode($delimiter, $col_default_value);
					$col_default_value = array_map('trim', $col_default_value);
				}

				$set_data = array(
					'field_name' => $csv_column_info['maps_to'],
					'value' => $col_value,
					'default_value' => $col_default_value,
					'temp_file_base_path' => $file_info['temp_file_base_path']
				);

				// ImporterEntityMetadataWrapper
				// public function set($field_name, $value, $force = TRUE) {
				$importer_wrapper->set(
					$set_data,
					TRUE 								// force or not force the data into.
				);

			} else {
				// drupal_set_message(t( 'The csv column '. $csv_col_name .' was not mapped to any field.' ));
			}
		} // close row loop

		/// BUILD THE INFORMATION TO BE DISPLAYED IN THE TABLE
		$entity_display = array();
		// the id
		if ( isset($identifier_field_name) && $identifier_field_name != '') {
			$entity_display[0] = $importer_wrapper->{$identifier_field_name}->value();
		} else {
			$entity_display[0] = 'Identifier field unknown.';
		}

		// the title
		$entity_display[1] = $importer_wrapper->title->value();



		////////////////////////////////////////////////
		/// FOURTH STEP: CREATE OR UPDATE THE ENTITY? //
		////////////////////////////////////////////////
		if ( $action == 'update' ) {
			
			array_push($to_update_displayers, $entity_display);

			array_push($to_update_wrappers, $importer_wrapper);

		} elseif ( $action == 'create' ) {

			array_push($to_create_displayers, $entity_display);

			array_push($to_create_wrappers, $importer_wrapper);

		}

	} // end csv while





	////////////////////////////////////////////////////////////////////////
	// FIFTH STEP: CHECK IF THE UPDATES AND CREATIONS HAVE BEEN CONFIRMED //
	////////////////////////////////////////////////////////////////////////
	if ( isset($step_data[4]['stored_values']) 
		&& isset($step_data[4]['stored_values']['final_confirm']) ) {

		$confirmations = $step_data[4]['stored_values']['final_confirm'];

		// if the confirmation has already been done...
		// SAVE !!!

		if ( isset($confirmations['confirm_create']) && $confirmations['confirm_create'] ) {
			foreach ($to_create_wrappers as $key => $wrapper) {
				$save_status = $wrapper->save();

				if ($save_status) {
					drupal_set_message(
						t('Entity @title was successfully created.',array('@title' => $wrapper->title->value()) )
					);
				} else {
					drupal_set_message(
						t('Entity @title was not created.',array('@title' => $wrapper->title->value()), 'error' )
					);
				}
			}
		}

		if ( isset($confirmations['confirm_update']) && $confirmations['confirm_update'] ){
			foreach ($to_update_wrappers as $key => $wrapper) {
				$save_status = $wrapper->save();

				if ($save_status) {
					drupal_set_message(
						t('Entity @title was successfully updated.',array('@title' => $wrapper->title->value()) )
					);
				} else {
					drupal_set_message(
						t('Entity @title was not updated.',array('@title' => $wrapper->title->value()), 'error' )
					);
				}
			}
		}


	} else {
		// if no confirmation has been set, build the table and the form.

		$to_create_table = array(
			'attributes' => array(),
			'caption' => 'Create the following entities',
			'colgroups' => array(),
			'sticky' => NULL,
			'empty' => 'Empty set',
		);
		$to_create_table['header'] = array('Identifier', 'Title');
		$to_create_table['rows'] = $to_create_displayers;

		$to_create_table = theme_table($to_create_table);


		$to_update_table = array(
			'attributes' => array(),
			'caption' => 'Update the following entities',
			'colgroups' => array(),
			'sticky' => NULL,
			'empty' => 'Empty set',
		);
		$to_update_table['header'] = array('Identifier', 'Title');
		$to_update_table['rows'] = $to_update_displayers;

		$to_update_table = theme_table($to_update_table);

		// build the form
		$form = array();

		$form['final_confirm'] = array(
			'#type' => 'fieldset',
			'#title' => 'Final confirmations',
			'#tree' => TRUE,
		);

		if ( count($to_create_displayers) > 0 ) {
			$form['final_confirm'] ['confirm_create'] = array(
				'#type' => 'checkbox',
				'#title' => 'Confirm creates',
				'#prefix' => $to_create_table,

				'#required' => TRUE,

				'#default_value' => 0,
			);
		}

		if ( count($to_update_displayers) > 0 ) {
			$form['final_confirm']['confirm_update'] = array(
				'#type' => 'checkbox',
				'#title' => 'Confirm updates',
				'#prefix' => $to_update_table,

				'#required' => TRUE,

				'#default_value' => 0,
			);
		}

		return $form;
	}
}






/**
 * Defines ImporterEntityMetadataWrapper
 */
class ImporterEntityMetadataWrapper  {

	// constructor 
	public function __construct($entity_type, $entity) {
		$this->wrapper = entity_metadata_wrapper($entity_type, $entity);
	}

	// Forward all requests for not found objects to the wrapper object.
	public function __get($prop) {
		return $this->wrapper->{$prop};
	}

	// Intercept all calls to methods not found on this object
	// and forward them to the wrapper object.
	public function __call($name, $arguments) {
		// forward the method calling
		return call_user_func_array(array($this->wrapper, $name),$arguments);
	}


	// set method
	public function set($data, $force = TRUE) {
		// $data: array(
		//	'value', 
		//	'default_value', 
		//	'field_name', 
		//	'temp_file_base_path'
		// )

		// VARS

		// make sure that the $value is always an array of values
		$value_arr = ( is_array($data['value']) ) ? $data['value'] : array($data['value']);

		// the field name
		$field_name = $data['field_name'];
		$field_info = field_info_field($field_name);
		// if there is no field info, that means that the field_name 
		// refers to a entity REAL property and not a entity field property
		// If this is the case, mock a field_info object
		if ( !isset($field_info) || $field_info == NULL ) {
			$field_info = array(
				'type' => 'title_string',
				'cardinality' => 1,
			);
		}

		// the base path for the file.. this was set on the form's first step.
		$temp_file_base_path = $data['temp_file_base_path'];

		// VARS


		///////////////////////
		// format the values //
		///////////////////////
		foreach ($value_arr as $key => $val) {

			$value_arr[$key] = call_user_func_array(
				array($this, 'format_value_to_field_type'),	// the method to call
				array($val, $field_info, $temp_file_base_path)
			);
		}

		// remove falsey value_arr from $value_arr
		$value_arr = array_filter($value_arr);
		///////////////////////
		// format the values //
		///////////////////////

		$field_is_single = ($field_info['cardinality'] == 1);

		// if the $value_arr is empty, 
		// instead of only interrupting the process, 
		// try to fill in the provided default value by calling this same method 
		// with a new data array.
		if ( count($value_arr) == 0 ) {

			if ( isset($data['default_value']) && $data['default_value'] != '' ) {
				$d = array(
					'value' => $data['default_value'],
					'default_value' => FALSE,
					'field_name' => $data['field_name'],
					'temp_file_base_path' => $data['temp_file_base_path']
				);

				return $this->set($d);
			} else {
				return FALSE;
			}
		}

		// fill stuff in.
		if ($field_is_single) {
			// remove the array wrapper
			// get the first value inside the array.
			$value = array_shift($value_arr);

			$this->wrapper->{$field_name}->set($value);

		} else {
			// the value is already an array, just set it

			$this->wrapper->{$field_name}->set($value_arr);
		}

	}

	// This function receives the CSV file columnd value,
	// an array containing field_info about the drupal field
	// and the base path for uploaded files 

	// and returns a correctly formatted value according to 
	// the field type (['type'] key in $field_info)
	public function format_value_to_field_type($value, $field_info, $temp_file_base_path) {

		switch( $field_info['type'] ) {
			case 'number_integer':
				$value = intval($value);
				break;

			case 'number_decimal':
				$value = floatval($value);
				break;

			case 'image':

				if ( !isset($value) || $value == '' ) {

					$value = FALSE;

				} else {

					$value = $temp_file_base_path . '/' . $value;
					$value = $this->format_file($value);

				}
				break;

			case 'file':
				if ( !isset($value) || $value != '' ) {
					$value = FALSE;

				} else {

					$value = $temp_file_base_path . '/' . $value;

					$value = $this->format_file($value);

				}
				break;

			case 'taxonomy_term_reference':
				$value = $this->format_taxonomy_term_reference($value, $field_info);
				break;

			case 'entityreference':
				$value = $this->format_entity_reference($value, $field_info);
				break;

			case 'date':
				$value = $this->format_date($value);
				break;

			case 'title_string':
				$value = isset($value) ? strval($value) : '';
				break;

			// defaults to string
			default: 
				$value = strval($value);
				break;

		}	// end switch

		return $value;
	}

	// builds a file array and returns it.
	// requires the filepath.
	public function format_file($filepath) {
		$fileexists = file_exists($filepath);

		if ($filepath && $fileexists) {
			// Create managed File object and associate with Image field.
			$file = (object) array(
				'uid' => 1,
				'uri' => $filepath,
				'filemime' => file_get_mimetype($filepath),
				'status' => 1,
				'display' => 1,
			);
			// We save the file to the root of the files directory.
			$file = file_copy($file, 'public://');

			return (array) $file;

		} else {
			drupal_set_message('File '. $filepath . ' does not exist. It was not uploaded.');
			return FALSE;
		}
	}

	// build date
	public function format_date($value) {
		// should return a varchar string with correct formatting.
	}

	// build the taxonomy term field
	public function format_taxonomy_term_reference($value, $field_info) {
		// value may either be the term id or the term value

		if ($value) {

			// first try loading by tid
			$term_load_by_tid = taxonomy_term_load($value);

			// if any term was successfully loaded by id,
			// the $tid to be referred to is the $value
			if ( isset($term_load_by_tid) && $term_load_by_tid ) {

				$ref_id = $value;

			} else {

				// try loading the term by name
				$term_load_by_name = taxonomy_get_term_by_name($value);

				if ( count($term_load_by_name) > 0 ) {
					$term_keys = array_keys($term_load_by_name);

					$ref_id = array_shift($term_keys);
				} else {
					$ref_id = FALSE;
				}

			}


			if ( isset($ref_id) && $ref_id != '' ) {
				return intval($ref_id);
			} else {
				return FALSE;
			}

		} else {
			return FALSE;
		}
	}

	public function format_entity_reference($value, $field_info) {
		// value may either be the entity id or the entity title value

		// first get the referenced entity type.
		$entity_type = $field_info['settings']['target_type'];

		if ($value) {

			// first try loading by entity id
			$entity_load_by_id = entity_load($entity_type, array($value));

			// if any entity was successfully loaded by id,
			// the $target_id is equal to the $value var.
			if ( count($entity_load_by_id) > 0 ) {
				$target_id = $value;
			} else {

				// try finding the entity by title
				$query = new EntityFieldQuery();

				$query->entityCondition('entity_type', $entity_type)
					->propertyCondition('title', $value);

				$results = $query->execute();
				// the pseudo-entity objects are within an array keyed by the entity type.
				$results = isset( $results[$entity_type] ) ? $results[$entity_type] : NULL ;

				if ( isset($results) && count($results) > 0) {
					// as we only expect one entity to be found with this value,
					// just get the first entity.
					$result_keys = array_keys($results);
					$first_result_id = array_shift($result_keys);

					$target_id = $first_result_id;
				} else {
					$target_id = FALSE;
				}

			}

			// if there is a $target_id, make sure it is an int.
			if (isset($target_id) && $target_id != '') {
				return intval($target_id);
			} else {
				return FALSE;
			}

		} else {
			return FALSE;	
		}
	}
}