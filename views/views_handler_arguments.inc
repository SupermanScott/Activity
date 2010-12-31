<?php
// $Id: $
/**
 * @file
 * Provide argument handlers for Activity module
 */
class activity_views_handler_argument_activity_user extends views_handler_argument_user_uid {
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
      '#required' => TRUE,
      '#default_value' => $this->options['realms'],
    );
  }

  function options_validate($form, &$form_state) {
    $form_state['values']['realms'] = array_filter($form_state['values']['realms']);
  }

  /**
   * Extract the value and enforce the realms restrictions.
   */
  function query() {
    $table = $this->ensure_my_table();
    if (!empty($this->options['break_phrase'])) {
      views_break_phrase($this->argument, $this);
    }
    else {
      $this->value = array($this->argument);
    }

    $allowed_realms = activity_cache_get('realms');
    $realm_values = array();

    foreach ($this->options['realms'] as $realm) {
      if (isset($allowed_realms[$realm])) {
        foreach ($this->value as $uid) {
          $realm_values += module_invoke($allowed_realms[$realm]['module'], 'activity_access_grants', user_load($uid));
        }
      }
    }

    // No valid values, probably an error but prevent anything from being shown.
    if (empty($realm_values)) {
      $realm_values = array('activity_none' => 1);
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