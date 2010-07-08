<?php
// $Id: $

/**
 * @file
 * Provides the Views Filter Handlers.
 */

/**
 * Filters the Activity records to those that are visible to the context user.
 *
 * @ingroup views_filter_handlers
 */
class activity_views_handler_filter_access extends views_handler_filter {  
  /**
   * Tell Views UI this filter can't be exposed.
   */
  function can_expose() {
    return FALSE;
  }

  function option_definition() {
    $options = parent::option_definition();
    $options['realms'] = array('default' => array());
    return $options;
  }

  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);
    foreach (activity_cache_get('realms') as $realm => $information) {
      $options[$realm] = t($information['name']);
    }
    $form['realms'] = array(
      '#title' => t('Realms'),
      '#type' => 'checkboxes',
      '#description' => t('Choose the realms to filter the Activity rows'),
      '#options' => $options,

      '#default_value' => $this->options['realms'],
    );
  }

  function options_validate($form, &$form_state) {
    $form_state['values']['realms'] = array_filter($form_state['values']['realms']);
  }

  function query() {
    module_load_include('inc', 'activity', 'modules/node.activity');
    $allowed_realms = activity_cache_get('realms');
    $realm_values = array();
    foreach ($this->options['realms'] as $realm) {
      if (isset($allowed_realms[$realm])) {
        $realm_values += module_invoke($allowed_realms[$realm]['module'], 'activity_access_grants', user_load(1));
      }
    }
  }
}
