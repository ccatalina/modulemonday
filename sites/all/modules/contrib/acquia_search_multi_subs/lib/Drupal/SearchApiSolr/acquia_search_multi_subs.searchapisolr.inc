<?php

/**
 * @file
 * Contains code specific to the Search API Solr Search module.
 */

/**
 * Initializes this module's Apache Solr Search Integration module support.
 *
 * Changes the Acquia Search environment on the fly based on the
 * AH_SITE_ENVIRONMENT and AH_SITE_NAME server variables.
 *
 * This will be called on every page request if the "search_api_acquia" module
 * is installed.
 *
 * @see acquia_search_multi_subs_init()
 */
function acquia_search_multi_subs_searchapi_init() {
  $servers = search_api_server_load_multiple(FALSE, array('class' => 'acquia_search_service'));
  // Loop over all Search API servers to look for the one we want to change.
  foreach ($servers as $server) {
    // Load defaults.
    $options = $server->options;
    $options += array('acquia_override_subscription' => array());
    $options['acquia_override_subscription'] += acquia_search_multi_subs_default_configuration();

    // If automatic switching is enabled.
    if ($options['acquia_override_subscription']['acquia_override_auto_switch']) {
       // Get the expected search core.
      $expected_core_name = acquia_search_multi_subs_get_expected_search_core();

      // Load our possible search cores.
      $search_cores = acquia_search_multi_subs_get_search_cores();

      // We know we have to switch automatically so we check if our core name
      // that is currently set is correct. If not, we change it and rebuild
      // our system.
      $environment_core_name = $options['acquia_override_subscription']['acquia_override_subscription_corename'];

      // If the core is already the same, don't do anything.
      if ($expected_core_name == $environment_core_name) {
        continue;
      }

      // Loop over all the possible search cores to find the one with the right
      // core_id.
      foreach ($search_cores as $search_core) {
        if ($search_core['core_id'] != $expected_core_name) {
          continue;
        }
        $server->options['acquia_override_subscription']['acquia_override_subscription_corename'] = $search_core['core_id'];
        // Get the derived key salt from the subscription.
        $subscription = acquia_agent_settings('acquia_subscription_data');
        $derived_key_salt = $subscription['derived_key_salt'];
        // Get the derived key salt from the subscription.
        $key = acquia_agent_settings('acquia_key');
        // Create our derived key.
        $derived_key = _acquia_search_multi_subs_create_derived_key($derived_key_salt, $search_core['core_id'], $key);
        $server->options['derived_key'] = $derived_key;

        $server->save();
        break;
      }
    }
  }
}

/**
 * Implements hook_search_api_service_info_alter().
 *
 * Overrides the service class used to connect to Acquia Search.
 */
function acquia_search_multi_subs_search_api_service_info_alter(array &$service_info) {
  $service_info['acquia_search_service']['class'] = 'SearchApiAcquiaSearchMultiService';
}
