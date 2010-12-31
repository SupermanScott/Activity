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
   * Tell Views UI this filter can't be exposed, yet.
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
      '#required' => TRUE,
    );
  }

  function options_validate($form, &$form_state) {
    $form_state['values']['realms'] = array_filter($form_state['values']['realms']);
  }

  function query() {
    $table = $this->ensure_my_table();
    $allowed_realms = activity_cache_get('realms');
    $realm_values = array();
    foreach ($this->options['realms'] as $realm) {
      if (isset($allowed_realms[$realm])) {
        $realm_values += module_invoke($allowed_realms[$realm]['module'], 'activity_access_grants', user_load($GLOBALS['user']->uid));
      }
    }

    if (empty($realm_values)) {
      $realm_values = array('activity_none' => array(1));
    }

    if (count($realm_values) > 1) {
      $grants = db_or();
      foreach ($realm_values as $realm => $ids) {
        $grants->condition(db_and()
          ->condition($table . '.realm', $realm)
          ->condition($table . '.value', $ids, 'IN'));
      }
      $this->query->add_where('AND', $grants);
    }
    else {
      $keys = array_keys($realm_values);
      $realm = $keys[0];
      $grants = db_and()
        ->condition($table . '.realm', $realm)
        ->condition($table . '.value', $realm_values[$realm]);
      $this->query->add_where(0, $grants);
    }
  }
}

class activity_views_handler_filter_actions_id extends views_handler_filter_in_operator {
  function get_value_options() {
    if (!isset($this->value_options)) {
      $this->value_title = t('Activity Template');
      $results = db_query("SELECT aid, label FROM {actions} WHERE type = 'activity'")
        ->fetchAllAssoc('aid');
 
      foreach ($results as $actions_id => $row) {
        $this->value_options[$actions_id] = check_plain($row->label);
      }
    }
  }
}