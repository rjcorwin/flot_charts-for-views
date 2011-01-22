<?php

class flot_charts_views_plugin_style extends views_plugin_style {
  function option_definition() {
    $options = parent::option_definition();
    $options['type'] = array('default' => 'line');
    $options['size'] = array('default' => '400x200');
    $options['x'] = array('default' => array('granularity' => 'auto', 'label' => 'default'));
    $options['y'] = array('default' => array('granularity' => 'auto', 'label' => 'default', 'pad' => 1));
    $options['layers'] = array('default' => array());
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

    // Generate label fields
    $label_options = array(
      '' => '< '. t('No labels') .' >',
      'default' => t('Default (from data points)'),
    );

    
    $form['x'] = array(
      '#tree' => TRUE,
      '#type' => 'fieldset',
      '#title' => t('X Axis'),
      '#collapsible' => TRUE,
      '#collapsed' => FALSE,
    );
    $form['x']['label'] = array(
      '#type' => 'select',
      '#options'=> $label_options,
      '#title' => t('Labels'),
      '#default_value' => $this->options['x']['label'],
    );
    $form['y'] = array(
      '#tree' => TRUE,
      '#type' => 'fieldset',
      '#title' => t('Y Axis'),
      '#collapsible' => TRUE,
      '#collapsed' => FALSE,
    );
    $form['y']['label'] = array(
      '#type' => 'select',
      '#options'=> $label_options,
      '#title' => t('Labels'),
      '#default_value' => $this->options['y']['label'],
    );
    $form['y']['pad'] = array(
      '#type' => 'checkbox',
      '#title' => t('Add headroom above points'),
      '#default_value' => $this->options['y']['pad'],
    );
    $form['y']['custom-y-max'] = array(
      '#type' => 'textfield',
      '#title' => t('Y Max'),
      '#size' => 15,
      '#description' => t("(Optional) Enter the Y axis Max."),
      '#default_value' => $this->options['y']['custom-y-max'],
    );
    $form['y']['custom-y-min'] = array(
      '#type' => 'textfield',
      '#title' => t('Y Min'),
      '#size' => 15,
      '#description' => t("(Optional) Enter the Y axis Min."),
      '#default_value' => $this->options['y']['custom-y-min'],
    );

    $default_colors = array('#666', '#999', '#ccc');
    $layer_0 = (isset($this->options['colors']['1'])) ? $this->options['colors']['1'] : '#666';
    $layer_1 = (isset($this->options['colors']['2'])) ? $this->options['colors']['2'] : '#999';
    $layer_2 = (isset($this->options['colors']['3'])) ? $this->options['colors']['3'] : '#ccc';

    $form['colors'] = array(
      '#tree' => TRUE,
      '#type' => 'fieldset',
      '#title' => t('Layer Colors'),
      '#collapsible' => TRUE,
      '#collapsed' => FALSE,
    );
    $form['colors']['0'] = array(
      '#type' => 'textfield',
      '#title' => t('Layer 1'),
      '#size' => 15,
      '#default_value' => $layer_0,
    );
    $form['colors']['1'] = array(
      '#type' => 'textfield',
      '#title' => t('Layer 2'),
      '#size' => 15,
      '#default_value' => $layer_1,
    );
    $form['colors']['2'] = array(
      '#type' => 'textfield',
      '#title' => t('Layer 3'),
      '#size' => 15,
      '#default_value' => $layer_2,
    );

    // Views
    $layers = $this->get_views_by_style();
    unset($layers["{$this->view->name}:{$this->view->current_display}"]);
    $form['layers'] = array(
      '#type' => 'checkboxes',
      '#title' => t('Additional data layers'),
      '#description' => t('Display the selected views displays as additional layers.'),
      '#options' => $layers,
      '#default_value' => $this->options['layers'],
    );
  }

  /**
   * Retrieve all views that have the specified style plugin.
   */
  protected function get_views_by_style($style_plugin = 'flot_charts') {
    $views = views_get_all_views();
    $usable = array();
    foreach ($views as $view) {
      foreach (array_keys($view->display) as $display) {
        $view->set_display($display);
        if ($view->display_handler->get_option('style_plugin') === $style_plugin) {
          $usable["{$view->name}:{$display}"] = "{$view->name}:{$display}";
        }
      }
      $view->destroy();
    }
    ksort($usable);
    return $usable;
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

    // Iterate over results to build data and ticks
    foreach ($result as $id => $row) {
      $datapoint = $view->field[$flot_field]->flot_render($row);
      $value = $datapoint['value'];
      $label = isset($datapoint['label']) ? $datapoint['label'] : $datapoint['value'];
      
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
    $type = !empty($options['type']) ? $options['type'] : 'line';
    $size = !empty($options['size']) ? explode('x', $options['size']) : array('200','100');

    // DOM element options
    $element = array();
    $element['style'] = is_numeric($size[0]) ? "width:{$size[0]}px;" : "width:{$size[0]};";
    $element['style'] .= is_numeric($size[1]) ? "height:{$size[1]}px;" : "height:{$size[1]};";
    $vars['element'] = $element;

    // Build layer.
    $layers = $this->build_layer($this->view, $this->view->result, $this->view->get_title());

    $vars['data'][] = $layers['series'];
    // $vars['data'][] = array( "label" => "far", "data" => $layers['series']);
    // $vars['data'][] = array( "data" => $layers['series']);
    $vars['titles'] = $layers['titles'];

    // Set up the type class, set axes
    switch ($options['type']) {
      case 'point':
        $style = new flotStylePoint();
        break;
      case 'bar':
        $style = new flotStyleBar();
        break;
      case 'line':
      default:
        $style = new flotStyleLine();
        break;
    }
    // Set colors.
    if (isset($options['colors']['0'])) {
        $style->colors[0] = $options['colors']['0'];
    }

    if (isset($options['colors']['1'])) {
        $style->colors[1] = $options['colors']['1'];
    }
    if (isset($options['colors']['2'])) {
        $style->colors[2] = $options['colors']['2'];
    }

    $vars['options'] = $style;
    $vars['options']->xaxis->mode = 'time';
    unset($vars['options']->xaxis->ticks);

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
