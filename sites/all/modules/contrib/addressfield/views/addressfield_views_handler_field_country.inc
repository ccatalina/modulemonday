<?php

class addressfield_views_handler_field_country extends views_handler_field {

  function option_definition() {
    $options = parent::option_definition();
    $options['human_readable'] = array('default' => TRUE);
    return $options;
  }

  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);

    $form['human_readable'] = array(
      '#type' => 'checkbox',
      '#title' => t('Display localized human readable country name instead of country code'),
      '#default_value' => $this->options['human_readable'],
    );
  }

  function get_value($values, $field = NULL) {
    $value = parent::get_value($values, $field);

    if (!empty($value)) {
      $countries = _addressfield_country_options_list();
      if (isset($countries[$value])) {
        $value = $countries[$value];
      }
    }

    return $value;
  }
}
