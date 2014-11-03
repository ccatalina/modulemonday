<?php

/**
 * @file
 * Helper functions used for the integration with either Solr module.
 */

/**
 * Calculates our expected search core name based on environment information.
 *
 * @return string
 *   The expected core's core_id.
 */
function acquia_search_multi_subs_get_expected_search_core() {
  // Check if we have the correct environment variables.
  if (isset($_ENV['AH_SITE_ENVIRONMENT'])) {
    $ah_site_environment = $_ENV['AH_SITE_ENVIRONMENT'];
  }
  else {
    // Default to dev.
    $ah_site_environment = 'dev';
  }
  // Load our possible search cores.
  $search_cores = acquia_search_multi_subs_get_search_cores();

  // Set the default core to production if all else fails.
  $subscription_id = acquia_agent_settings('acquia_identifier');
  $expected_core_name = $subscription_id;

  // Without a sitegroup value we can't do much.
  if (!isset($_ENV['AH_SITE_NAME'])) {
    // We did not find a sitename, so we default to a dev search core if one is
    // present.
    foreach ($search_cores as $search_core) {
      if (strstr($search_core['core_id'], 'dev')) {
        $expected_core_name = $search_core['core_id'];
        break;
      }
    }
  }
  else {
    // In all other cases, we see if we can find a core to default to.
    $ah_site_name = $_ENV['AH_SITE_NAME'];
    // Replace all weird characters that might appear in the sitegroup name or
    // identifier.
    $ah_site_name = preg_replace('@[^a-zA-Z0-9_-]+@','_',$ah_site_name);

    // Generate our expected core name according to Acquia Search
    // conventions.
    if ($ah_site_environment != 'prod') {
      $expected_core_name = $subscription_id . '_' . $ah_site_name;
    }
  }
  return $expected_core_name;
}

/**
 * Retrieves all the available search cores as set in the subscription.
 *
 * @return array
 *   The search cores that are available according to the information in the
 *   rpc backend.
 */
function acquia_search_multi_subs_get_search_cores() {
  $search_cores = drupal_static(__FUNCTION__, array());

  // See if we can return it from static cache.
  if (!empty($search_cores)) {
    return $search_cores;
  }

  $subscription = acquia_agent_settings('acquia_subscription_data');
  // Get our search cores if they exist.
  if (isset($subscription['heartbeat_data']['search_cores'])) {
    $search_cores = $subscription['heartbeat_data']['search_cores'];
  }
  return $search_cores;
}

/**
 * Form constructor for configuring the Solr server switch.
 *
 * Used by SearchApiAcquiaSearchMultiService::configurationForm() and
 * acquia_search_multi_subs_form_apachesolr_environment_edit_form_alter() to add
 * these options in the appropriate place for the different modules.
 *
 * @param array $form
 *   The existing form's array, passed as a reference to be appended to.
 * @param array $form_state
 *   The form's current state.
 * @param array $configuration
 *   The existing configuration for this environment/server.
 */
function acquia_search_multi_subs_get_settings_form(array &$form, array &$form_state, array $configuration) {
  // Fill in defaults.
  $configuration += acquia_search_multi_subs_default_configuration();

  // Define the override form.
  $form['acquia_override_subscription'] = array(
    '#type' => 'fieldset',
    '#title' => t('Configure Acquia Search'),
    '#description' => t('This is usually not necessary unless you really want this search environment to connect to a different Acquia search subscription. By default it uses your subscription that was configured in the <a href="@url">Acquia Agent</a>.', array('@url' => url('admin/config/system/acquia-agent'))),
    '#collapsed' => FALSE,
    '#collapsible' => TRUE,
    '#tree' => TRUE,
    '#weight' => 11,
    '#element_validate' => array('acquia_search_multi_subs_form_validate'),
  );

  // Add a checkbox to auto switch per environment.
  $form['acquia_override_subscription']['acquia_override_auto_switch'] = array(
    '#type' => 'checkbox',
    '#title' => t('Automatically switch when an Acquia Environment is detected'),
    '#description' => t('Based on the detection of the AH_SITE_NAME and AH_SITE_ENVIRONMENT header we can detect which environment you are currently using and switch the Acquia Search Core automatically if there is a corresponding core.'),
    '#default_value' => $configuration['acquia_override_auto_switch'],
  );

  // Add a form element to make it easier to choose from multiple cores.
  $options = array('other' => t('Other'));
  $search_cores = acquia_search_multi_subs_get_search_cores();
  if (is_array($search_cores)) {
    foreach ($search_cores as $search_core) {
      $options[$search_core['core_id']] = $search_core['core_id'];
    }
  }
  $form['acquia_override_subscription']['acquia_override_selector'] = array(
    '#type' => 'select',
    '#title' => t('Acquia Search Core'),
    '#options' => $options,
    '#default_value' => $configuration['acquia_override_selector'],
    '#description' => t('Choose any of the available search cores or manually define one by choosing "other".'),
    '#states' => array(
      'visible' => array(
        ':input[name*="acquia_override_auto_switch"]' => array('checked' => FALSE),
      ),
    ),
  );
  // Show a warning if there are not enough cores available to make the auto
  // switch possible.
  if (count($options) <= 2) {
    drupal_set_message(t('It seems you only have 1 Acquia Search index. To find out if you are eligible for a search core per environment it is recommended you open a support ticket with Acquia.'), 'warning', FALSE);
  }

  // Generate the custom form.

  $form['acquia_override_subscription']['acquia_override_subscription_id'] = array(
    '#type' => 'textfield',
    '#title' => t('Enter your Acquia Subscription Identifier'),
    '#description' => t('Prefilled with the identifier of the Acquia Connector. You can find your details in Acquia Insight.'),
    '#default_value' => $configuration['acquia_override_subscription_id'],
    '#states' => array(
      'visible' => array(
        ':input[name*="acquia_override_selector"]' => array('value' => 'other'),
        ':input[name*="acquia_override_auto_switch"]' => array('checked' => FALSE),
      ),
    ),
  );

  $form['acquia_override_subscription']['acquia_override_subscription_key'] = array(
    '#type' => 'textfield',
    '#title' => t('Enter your Acquia Subscription key'),
    '#description' => t('Prefilled with the key of the Acquia Connector. You can find your details in Acquia Insight.'),
    '#default_value' => $configuration['acquia_override_subscription_key'],
    '#states' => array(
      'visible' => array(
        ':input[name*="acquia_override_selector"]' => array('value' => 'other'),
        ':input[name*="acquia_override_auto_switch"]' => array('checked' => FALSE),
      ),
    ),
  );

  $form['acquia_override_subscription']['acquia_override_subscription_corename'] = array(
    '#type' => 'textfield',
    '#description' => t('Please enter the name of the Acquia Search core you want to connect to that belongs to the above identifier and key. In most cases you would want to use the dropdown list to get the correct value.'),
    '#title' => t('Enter your Acquia Search Core Name'),
    '#default_value' => $configuration['acquia_override_subscription_corename'],
    '#states' => array(
      'visible' => array(
        ':input[name*="acquia_override_selector"]' => array('value' => 'other'),
        ':input[name*="acquia_override_auto_switch"]' => array('checked' => FALSE),
      ),
    ),
  );
}

/**
 * Returns default configuration for the override functionality.
 *
 * @return array
 *   The default configuration.
 */
function acquia_search_multi_subs_default_configuration() {
  return array(
    'acquia_override_auto_switch' => TRUE,
    'acquia_override_selector' => '',
    'acquia_override_subscription_id' => acquia_agent_settings('acquia_identifier'),
    'acquia_override_subscription_key' => acquia_agent_settings('acquia_key'),
    'acquia_override_subscription_corename' => '',
  );
}

/**
 * Form element validation handler for the subscription override sub-form.
 *
 * @see acquia_search_multi_subs_get_settings_form()
 */
function acquia_search_multi_subs_form_validate(array $element, &$form_state) {
  // Get the correct values array.
  $form_values = &$form_state['values'];
  foreach ($element['#parents'] as $key) {
    $form_values = &$form_values[$key];
  }

  // Don't check the settings further if we are set to "auto" mode.
  if ($form_values['acquia_override_auto_switch']) {
    // Already set the correct ID and key, though, since those won't change.
    // Only the core name is adapted automatically.
    $form_values['acquia_override_subscription_id'] = acquia_agent_settings('acquia_identifier');
    $form_values['acquia_override_subscription_key'] = acquia_agent_settings('acquia_key');
    return;
  }

  // If the selector is set to a core (not "other"), insert the proper settings.
  if ($form_values['acquia_override_selector'] == 'other') {
    if ($form_values['acquia_override_subscription_id'] == '') {
      form_error($element['acquia_override_subscription_id'], t('You must at least fill in a valid Acquia Subscription Identifier.'));
    }
    if ($form_values['acquia_override_subscription_key'] == '') {
      form_error($element['acquia_override_subscription_key'], t('You must at least fill in a valid Acquia Subscription key.'));
    }
    if ($form_values['acquia_override_subscription_corename'] == '') {
      $form_values['acquia_override_subscription_corename'] = $form_values['acquia_override_subscription_id'];
    }
  }
  else {
    $form_values['acquia_override_subscription_id'] = acquia_agent_settings('acquia_identifier');
    $form_values['acquia_override_subscription_key'] = acquia_agent_settings('acquia_key');
    $form_values['acquia_override_subscription_corename'] = $form_values['acquia_override_selector'];
  }

  // Set the identifier and key.
  $identifier = $form_values['acquia_override_subscription_id'];
  $key = $form_values['acquia_override_subscription_key'];

  $subscription = acquia_agent_get_subscription(array(), $identifier, $key);
  if (!is_array($subscription)) {
    form_error($element['acquia_override_subscription_key'], t('This combination of ID and key is not valid.'));
  }
}

/**
 * Retrieves the host name to use for a certain Acquia Search core.
 *
 * @param string $corename
 *   The core_id of the Acquia Search core.
 *
 * @return string
 *   The domain to use for this core.
 */
function acquia_search_multi_subs_get_hostname($corename) {
  // Override the URL.
  $search_host = variable_get('acquia_search_host', 'search.acquia.com');
  $search_cores = acquia_search_multi_subs_get_search_cores();
  foreach ($search_cores as $search_core) {
    if ($corename == $search_core['core_id']) {
      $search_host = $search_core['balancer'];
    }
  }
  return $search_host;
}

/**
 * Derives a key for the solr hmac using a salt, id and key.
 *
 * Identical copy of _acquia_search_create_derived_key(), to avoid a dependency
 * on the acquia_search module.
 */
function _acquia_search_multi_subs_create_derived_key($salt, $id, $key) {
  $derivation_string = $id . 'solr' . $salt;
  return hash_hmac('sha1', str_pad($derivation_string, 80, $derivation_string), $key);
}
