<?php
// $Id: $

/**
 * @file
 * Provides the Views Field Handlers.
 */

/**
 * Renders the message field for a given user
 *
 * @ingroup views_field_handlers
 */
class activity_views_handler_field_message extends views_handler_field {
  /**
   * Override the query method to add the second table.
   */
  function query() {
    $this->ensure_my_table();
    
    // Join in the personal message table.
    $this->personal_msg_table = $this->query->ensure_table('activity_personal_messages', $this->relationship);
    
    // Use the personal message if there is one otherwise use the non-personal.
    $this->field_alias = $this->query->add_field(NULL, "COALESCE($this->personal_msg_table.message, $this->table_alias.$this->real_field)", $this->table_alias . '_' . $this->real_field);
  }

  function render($values) {
    // The admin is allowed to enter html into the markup so filter that here.
    return filter_xss_admin($values->{$this->field_alias});
  }
}

/**
 * Presents a link to delete an activity.
 *
 * @ingroup views_field_handlers
 */
class activity_views_handler_field_activity_link_delete extends views_handler_field {
  function option_definition() {
    $options = parent::option_definition();
    $options['text'] = array('default' => '', 'translatable' => TRUE);

    return $options;
  }

  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);
    $form['text'] = array(
      '#type' => 'textfield',
      '#title' => t('Text to display'),
      '#default_value' => $this->options['text'],
    );
  }

  function render($values) {
    if (activity_delete_access($values->{$this->field_alias})) {
      $text = !empty($this->options['text']) ? $this->options['text'] : t('delete');
      return l($text, "activity/" . $values->{$this->field_alias} . "/delete", array('query' => drupal_get_destination()));
    }
    else {
      return;
    }
  }
}

/**
 * Class to display the admin name for the hook.
 */
class activity_views_handler_field_type extends views_handler_field {
  function render($values) {
    $hooks = activity_cache_get('hooks');
    if (isset($hooks[$values->{$this->field_alias}])) {
      return t($hooks[$values->{$this->field_alias}]['name']);
    }
    else {
      return parent::render($values);
    }
  }
}
