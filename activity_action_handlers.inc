<?php
// $Id: $

/**
 * @ingroup activity handlers
 * @{
 */
abstract class ActivityActionHandler {
  /**
   * Templates created by the administrator.
   *
   * @var array
   */
  public $templates = array();

  /**
   * Options provided vai the optionsFrom and optionsDefinition methods.
   *
   * @var array
   */
  public $options = array();

  /**
   * Type of activity.
   *
   * @var string
   */
  public $type = 'activity';

  /**
   * {actions}.aid for the template.
   *
   * @var integer
   */
  public $actions_id = 0;

  /**
   * {actions}.label for the template.
   *
   * @var string
   */
  public $label;

  /**
   * Load objects for tokenization.
   *
   * @param &$objects
   *   An array of objects.
   *
   * @return array
   *  
   */
  public function loadObjects($context) {
    $objects = array();
    if (isset($context['account'])) {
      $objects['user'] = $context['account'];
    }

    return $objects;
  }

  /**
   * Return option defaults and structure.
   *
   * @return array.
   */
  public function optionDefinition() {
  }

  /**
   * Display an FAPI form.
   *
   * @param &$form
   *   An FAPI form array.
   * @param $form_state
   *   The form_state from FAPI.
   */
  public function optionForm(&$form, $form_state) {
  }

  /**
   * Determine if the current Action is valid for this object.
   *
   * @param array $objects
   *   The collection of objects for this action.
   * @param array $context
   *   The context array passed into by the triggering module.
   *
   * @return boolean
   */
  public function valid($objects, $context) {
    return TRUE;
  }

  /**
   * Display the token message form.
   *
   * @param &$form
   *   The FAPI form.
   * @param $form_state
   *   The state of the form.
   */
  public function messagesForm(&$form, $form_state) {
    $messages = $this->messages() + array(
      'public' => array(
        'title' => 'Public Message',
        'description' => 'Message displayed to everyone who is not a part of this Activity',
      ),
    );
    foreach (activity_enabled_languages() as $id => $language) {
      $form[$id] = array(
        '#type' => 'fieldset',
        '#title' => t('@name messages', array('@name' => $language->name)),
      );
      foreach ($messages as $object_key => $information) {
        $form[$id][$object_key] = array(
          '#type' => 'textarea',
          '#title' => t($information['title']),
          '#description' => t($information['description']),
          '#default_value' => $this->templates[$id][$object_key],
        );
      }
    }

    // The token.module provides the UI for the tokens. While not required,
    // it adds a nice UI.
    if (module_exists('token')) {
      $form['token_help'] = array(
        '#theme' => 'token_tree',
        '#token_types' => array_keys($messages),
      );
    }
  }

  /**
   * Return an array of message types.
   */
  protected function messages() {
    return array();
  }

  /**
   * Tokenize based on the provided objects
   *
   * @param array $objects
   *  The objects used in the tokenization process.
   *
   * @return array
   *  An array of messages keyed by language and uid.
   */
  public function tokenize($objects) {
    $messages = array();
    foreach ($this->templates as $language_id => $language_patterns) {
      foreach ($language_patterns as $target_key => $text) {
        // @TODO: pass in the language object.
        $message = token_replace($text, $objects, array('clear' => TRUE, 'sanitize' => FALSE));
        if (!empty($message)) {
          $messages[$language_id][$this->get_uid($target_key, $objects)] = $message;
        }
      }
    }

    return $messages;
  }

  /**
   * Return the user id that matches the provided key in the templates.
   *
   * @param string $key
   *   The message key.
   * @param array $objects
   *   All the objects for the Activity.
   *
   * @return int
   */
  protected function get_uid($key, $objects) {
    if ($key == 'public') {
      return 0;
    }
    elseif ($key == 'current_user') {
      return $GLOBALS['user']->uid;
    }
    return $objects[$key]->uid;
  }

  /**
   * Return the nid of this Activity.
   *
   * @param array $objects
   *  An array of objects used in tokenization.
   *
   * @return int / NULL
   */
  public function determineNid($objects) {
    $nid = NULL;
    if (isset($objects['node']->nid)) {
      $nid = $objects['node']->nid;
    }
  
    // NOTICE: no elseif(). This is because if the comment is part of the
    // objects, use that as the basis for the nid. Doubt that will ever be an
    // issue.
    if (isset($objects['comment']->nid)) {
      $nid = $objects['comment']->nid;
    }

    return $nid;
  }

  /**
   * Return the eid field for this Activity.
   *
   * @param array $objects
   *  The objects used in tokenization
   *
   * @return int / NULL
   */
  public function determineEid($objects) {
    return NULL;
  }

  /**
   * Return the uid that is responsible for this action.
   *
   * @param array $objects
   *  The objects used in tokenization
   *
   * @return int
   */
  public function determineActor($objects) {
    return $GLOBALS['user']->uid;
  }
}

/**
 * Activity handler for node module.
 */
class NodeActivityActionHandler extends ActivityActionHandler {
  /**
   * Return the eid field for this Activity.
   *
   * @param array $objects
   *  The objects used in tokenization
   *
   * @return int / NULL
   */
  public function determineEid($objects) {
    return $objects['node']->nid;
  }

  /**
   * Return the uid that is responsible for this action.
   *
   * @param array $objects
   *  The objects used in tokenization
   *
   * @return int
   */
  public function determineActor($objects) {
    return $objects['node']->uid;
  }

  /**
   * Load objects for tokenization.
   *
   * @param &$objects
   *   An array of objects.
   *
   * @return array
   *  
   */
  public function loadObjects($context) {
    $objects = parent::loadObjects($context);
    $objects['node'] = $context['node'];

    return $objects;
  }

  /**
   * Display an FAPI form.
   *
   * @param &$form
   *   An FAPI form array.
   * @param $form_state
   *   The form_state from FAPI.
   */
  public function optionForm(&$form, $form_state) {
    parent::optionForm($form, $form_state);
    $node_types = array();

    foreach (node_type_get_types() as $type) {
      $node_types[$type->type] = check_plain($type->name);
    }

    $form['types'] = array(
      '#type' => 'checkboxes',
      '#title' => t('Allowed Node Types'),
      '#options' => $node_types,
      '#default_value' => $this->options['types'],
    );

    $node_entity_info = entity_get_info('node');
    $view_mode_options = array();
    foreach ($node_entity_info['view modes'] as $mode => $information) {
      $view_mode_options[$mode] = $information['label'];
    }

    // This is a node_view specific option.
    $form['view_modes'] = array(
      '#type' => 'checkboxes',
      '#title' => t('View Modes'),
      '#options' => $view_mode_options,
      '#default_value' => $this->options['view_modes'],
      '#access' => $this->type == 'node_view',
    );
  }

  /**
   * Determine if the current Action is valid for this object.
   *
   * @param array $objects
   *   The collection of objects for this action.
   * @param array $context
   *   The context array passed into by the triggering module.
   *
   * @return boolean
   */
  public function valid($objects, $context) {
    $types = array_filter($this->options['types']);
    $type_check = TRUE;
    $view_modes_check = TRUE;

    if (!empty($types)) {
      $type_check = in_array($objects['node']->type, $types);
    }

    $view_modes = array_filter($this->options['view_modes']);
    if (!empty($view_modes)) {
      // When the trigger is fired, the build mode is passed to the actions
      // callback as $a1. activity_record() takes the two arguments and places
      // them into the context array under the 'additional_arguments' key.
      // @see activity_record
      $view_modes_check = in_array($context['additional_arguments'][0], $view_modes);
    }
    return $type_check && $view_modes_check;
  }

  /**
   * Return an array of message types.
   */
  protected function messages() {
    $messages = parent::messages();
    $messages['node'] = array(
      'title' => 'Node Author',
      'description' => 'Author of the node or the user who updated the node',
    );

    if ($this->type == 'node_update') {
      // If node->uid == $GLOBALS['user']->uid, this message template will be
      // used instead of $messages['node'].
      // This key 'current_user' is a fake one, and its existance is handled in
      // get_uid() method.
      $messages['current_user'] = array(
        'title' => 'Updating User',
        'description' => 'The user who actually updated the node',
      );
    }

    return $messages;
  }
}

/**
 * @} End of "ingroup activity handlers".
 */