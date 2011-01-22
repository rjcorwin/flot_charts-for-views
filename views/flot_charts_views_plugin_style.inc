<?php

class flot_charts_views_plugin_style extends views_plugin_style {
  function option_definition() {
    $options = parent::option_definition();
    $options['type'] = array('default' => 'line');
    $options['size'] = array('default' => '400x200');
    $options['x-axis']['time-mode'] = array('default' => FALSE);
    $options['manual_options']['json'] = array('default' => 'null');
    $options['manual_options']['use_json'] = array('default' => FALSE);
    return $options;
  }

  function options_form(&$form, &$form_state) {

    $form['type'] = array(
      '#type' => 'select',
      '#title' => t('Graph type'),
      '#options' => array('line' => t('Line'), 'bar' => t('Bar'), 'point' => t('Point')),
      '#description' => t("Choose the type of chart you would like to display."),
      '#default_value' => $this->options['type'],
    );

    $form['size'] = array(
      '#type' => 'textfield',
      '#title' => t('Size'),
      '#description' => t("Enter the dimensions for the chart. Format: WIDTHxHEIGHT (e.g. 200x100)"),
      '#default_value' => $this->options['size'],
    );

    $form['x-axis'] = array(
      '#tree' => TRUE,
      '#type' => 'fieldset',
      '#title' => t('X axis'),
      '#collapsible' => TRUE,
      '#collapsed' => FALSE,
    );

    $form['x-axis']['time-mode'] = array(
      '#type' => 'checkbox',
      '#title' => t('Use time mode'),
      '#default_value' => $this->options['x-axis']['time-mode'],
    );

    $form['manual_options'] = array(
      '#tree' => TRUE,
      '#type' => 'fieldset',
      '#title' => t('Manual Options'),
      '#collapsible' => TRUE,
      '#collapsed' => FALSE,
    );

    $form['manual_options']['json'] = array(
      '#type' => 'textarea',
      '#cols' => 60,
      '#rows' => 5,
      '#title' => t('Manual Options JSON'),
      '#description' => t("Manually enter a JSON object for the options object bound for Flot's .plot() function.
        See <a href='http://flot.googlecode.com/svn/trunk/API.txt'>the Flot API documentation</a> for a list of all options."),
      '#default_value' => $this->options['manual_options']['array'],
    );

    $form['manual_options']['use_json'] = array(
      '#type' => 'checkbox',
      '#title' => t('Override all options with the Manual Options JSON.'),
      '#default_value' => $this->options['manual_options']['use_json'],
    );

    // TODO x and y min + max with option to expose
     

  }

  protected function build_layer($view, $result, $title = NULL) {
    // Get flot field, and bail if not present.
    if (isset($view->style_plugin) && method_exists($view->style_plugin, 'get_flot_field')) {
      $flot_field = $view->style_plugin->get_flot_field();
    }
    else {
      return;
    }

    // Make sure there is a result set otherwise exit
    if (!$result) {
      return;
    }

    // Iterate over results to build data
    foreach ($result as $id => $row) {
      $datapoint = $view->field[$flot_field]->flot_render($row);
      $value = $datapoint['value']; 
      $layer['series'][] = array($value[0], $value[1]);
    }
    $layer['series'] = new flotData($layer['series']);
    
    return $layer;
  }


  /**
   * Theme template preprocessor.
   */
  function preprocess(&$vars) {
    $view = $this->view;
    $options = $this->options;

    // Parameters
    $size = !empty($options['size']) ? explode('x', $options['size']) : array('200','100');

    // DOM element options
    $element = array();
    $element['style'] = is_numeric($size[0]) ? "width:{$size[0]}px;" : "width:{$size[0]};";
    $element['style'] .= is_numeric($size[1]) ? "height:{$size[1]}px;" : "height:{$size[1]};";
    $vars['element'] = $element;

    // Build layer.
    // TODO build_layer(s) could be our function to sort through the data into groups
    $layers = $this->build_layer($this->view, $this->view->result, $this->view->get_title());

    // $vars['data'][] = $layers['series'];
    // $vars['data'][] = array( "label" => "far", "data" => $layers['series']->data);
    $vars['data'][] = array( "data" => $layers['series']->data);

    // Set up the type class, set axes
    switch ($options['type']) {
      case 'point':
        $flot_options = new flotStylePoint();
        break;
      case 'bar':
        $flot_options = new flotStyleBar();
        break;
      case 'line':
      default:
        $flot_options = new flotStyleLine();
        break;
    }

    // Set x-axis to time mode if chosen
    $flot_options->xaxis->mode = !empty($options['x-axis']['time-mode']) ? 'time' : NULL;

    // Decode the manually entered JSON object if set to do so, otherwise set $vars['options'] to $flot_options
    $json_decoded = !empty($options['manual_options']['use_json']) ? json_decode($options['manual_options']['json']) : TRUE;
    if(!$json_decoded) {
      drupal_set_message("A manually inserted json object (in the Flot Charts Views style) failed to validate using PHP's json_decode() function.", 'error');
    }
    $vars['options'] = !empty($options['manual_options']['use_json']) ? $json_decoded : $flot_options;
    
  }

  /**
   * Validate function.
   */
  function validate() {
    parent::validate();
    $field = $this->get_flot_field();
    if (!$field) {
      return array(t('You must use a field that is compatible (e.g. <strong>Data point</strong>) with Flot to use the Flot style plugin.'));
    }
  }

  /**
   * Get the first usable flot field on this view.
   */
  function get_flot_field() {
    $fields = $this->display->handler->get_option('fields');
    foreach ($fields as $field => $info) {
      $handler = get_class(views_get_handler($info['table'], $info['field'], 'field'));
      if (method_exists($handler, 'flot_render')) {
        return $field;
      }
    }
    return FALSE;
  }
}
