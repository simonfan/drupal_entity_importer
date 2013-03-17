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

	$file_info = $step_data[1]['stored_values'];
	$identifier_field_name = $step_data[2]['stored_values']['identifier_field_name'];
	$csv_col_names = $step_data[2]['stored_values']['csv_col_names'];
	$csv_map = $step_data[2]['stored_values']['csv_map'];

	// first load the csv file.
	// get the first line of the CSV file
	$delimiter = $file_info['csv_delimiter'];
	$enclosure = $file_info['csv_enclosure'];

	$file_obj = file_load($file_info['file']);
	$file_resource = fopen($file_obj->uri, 'r');

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
		$importer_wrapper = new ImporterEntityMetadataWrapper($file_info['entity_type'], $entity );






		/////////////////////////////////////////////////////
		////// THIRD STEP: SET THE VALUES ON THE WRAPPER ////
		/////////////////////////////////////////////////////


// echo '<pre>';
		// loop through the row columns
		foreach ($row_columns as $col_no => $col_value) {

			// get the col_name
			$csv_col_name = $csv_col_names[ $col_no ];

			// get the map info object
			$csv_column_info = $csv_map[ $csv_col_name ];

/*

			echo '<br><br><br>';
			echo 'col value before: ';
			var_dump($col_value);

			var_dump($csv_column_info);
*/
			// only set on the importer_wrapper if
			// the csv_column was mapped to any field
			
			if ( isset($csv_column_info['maps_to']) && $csv_column_info['maps_to'] != NULL ) {

				// check if the column_value is a multiple value.
				if ($csv_column_info['is_multiple']) {

$finfo = field_info_field( $csv_column_info['maps_to'] );

if ($finfo['type'] == 'image' ) {
	echo $csv_column_info['is_multiple'];
	die();
}

					$delimiter = $csv_column_info['subfield_delimiter'];

					$col_value = explode($delimiter, $col_value);

					$col_value = array_map('trim', $col_value);
				}

				// if the field_type is 'file' or 'image',
				// transform the $val in a path
				$finfo = field_info_field( $csv_column_info['maps_to'] );

				if ($finfo['type'] == 'image' || $finfo['type'] == 'file') {
					if ( isset($col_value) && $col_value != '') {

						if ( is_array($col_value) ) {
							__dump($col_value);
						}

						// only build the path if the file is not null
						$col_value = $file_info['temp_file_base_path'] . '/' . $col_value;
					}
				}

				// ImporterEntityMetadataWrapper
				// public function set($field_name, $value, $force = TRUE) {
				$importer_wrapper->set(
					$csv_column_info['maps_to'],		// field_name
					$col_value,							// the value to set the field to
					TRUE 								// force or not force the data into.
				);

			} else {
//				drupal_set_message(t( 'The csv column '. $csv_col_name .' was not mapped to any field.' ));
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


drupal_set_message(t('<pre>@e</pre>', array('@e' => print_r($step_data, TRUE))));


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

//	return array('to_create' => $to_create, 'to_update' => $to_update);
/*
echo 'to create';
	echo '<pre>';

	var_dump($to_create);

	echo '</pre>';

echo 'to update';

echo '<pre>';

var_dump($to_update);

echo '</pre>';
die();

*/
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
	public function set($field_name, $value, $force = TRUE) {
		// $prop: the name of the property tobe set on the wrapper
		// $value: the value to be set to. might be array or string
		// $force: is used in case the $value format does not fit the $prop format requirements
		//			in that case, if $force is set to true, the value will be modified to fit in

		// check the value data type
		$value_is_single = !is_array($value);

		// check the field data type
		$field_format = $this::field_data_format($field_name);
		$field_is_single = $field_format['is_single'];

		///////////////////////
		// format the values //
		///////////////////////
		if ($value_is_single) {
			$value = $this->format_to_field_type($value, $field_format['type']);

			drupal_set_message('The value type is '. gettype($value) . ' and the required type is '. $field_format['type']);

			// if the $value gets to be FALSE, return
			if (!$value) {
				return;
			}


		} else {

			foreach ($value as $key => $val) {
				$value[$key] = call_user_func_array(
					array($this, 'format_to_field_type'),	// the method to call
					array($val, $field_format['type'])
				);
			}

			// filter the array from falsey values
			array_filter($value);
		}
		///////////////////////
		// format the values //
		///////////////////////



		// fill them in.

		if ($field_is_single && $value_is_single) {
			// just fill in
			$this->wrapper->{$field_name}->set($value);

		} elseif ($field_is_single && !$value_is_single) {
			// throw alert message
			drupal_set_message(t('The column value '. $value .' is a multi-value (according to your settings, but the field '. $field_name .' is not. Anyway, the first value from the $value was used instead.'), 'error');

			// get the first value inside the array.
			$value = array_shift($value);

			$this->wrapper->{$field_name}->set($value);

		} elseif (!$field_is_single && $value_is_single) {
			// put the value inside an array and set it to the field.
			$value = array($value);

			$this->wrapper->{$field_name}->set($value);

		} elseif (!$field_is_single && !$value_is_single) {
			// just fill in
			$this->wrapper->{$field_name}->set($value);
		}

	}
	
	// method that determines if a given field receives signle or multiple values
	public function field_data_format($field_name) {
		$answer = array();

		$finfo = field_info_field($field_name);

		if ( isset($finfo) && count($finfo) > 0 ) {

			$answer['is_field'] = TRUE;

			$answer['field_info'] = $finfo;
			$answer['is_single'] = ($finfo['cardinality'] == 1);
			$answer['type'] = $finfo['type'];

		} else {
			// if there is no field info, that means the requested field 
			// is actually a property
			$answer['is_field'] = FALSE;

			$answer['field_info'] = NULL;
			$answer['is_single'] = TRUE;


			// AS this is not a field, assume that the correct type is 
			// string
			$answer['type'] = 'string';

		}

		return $answer;
	}


	public function format_to_field_type($var, $type) {
		switch($type) {
			case 'number_integer':
				return intval($var);
				break;

			case 'number_decimal':
				return floatval($var);
				break;

			case 'image':
				return $this->format_file($var);
				break;

			case 'file':
				return $this->format_file($var);
				break;

			// defaults to string
			default: 
				return strval($var);
				break;
		}
	}

	// builds a file array and returns it.
	// requires the filepath.
	public function format_file($filepath) {
		if ($filepath) {
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
			return FALSE;
		}
	}
}