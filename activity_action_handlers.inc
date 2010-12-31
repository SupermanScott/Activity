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
   * Options provided via the optionsFrom and optionsDefinition methods.
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
   * Boolean indicating if this handler can do batch operations.
   *
   * @var boolean
   */
  public $batch;

  /**
   * Generates the default options from the provided option definition.
   *
   * @param array $definition
   *   Option definition from the handler.
   *
   * @return array
   *   Default options.
   */
  public static function defaultOptions(array $definition) {
    $accumlator = array();
    foreach (element_children($definition) as $child) {
      if (isset($definition[$child]['#default_value'])) {
        $accumlator[$child] = $definition[$child]['#default_value'];
      }
      else {
        $grand_children = element_children($definition[$child]);
        if (!empty($grand_children)) {
          $accumlator[$child] = self::defaultOptions($definition[$child]);
        }
      }
    }
    return $accumlator;
  }

  /**
   * Load objects for tokenization.
   *
   * @param int $eid
   *   The entity identifier for this Activity.
   *
   * @return array
   */
  abstract public function loadObjects($eid);

  /**
   * Return option defaults and structure.
   *
   * @return array.
   */
  public function optionDefinition() {
    return array(
      'window' => array(
        '#default_value' => 0,
      ),
    );
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
    $options = drupal_map_assoc(range(0, 7200, 300), 'format_interval');
    $options[0] = t('Unlimited');
    $form['window'] = array(
      '#type' => 'select',
      '#title' => t('Activity Window'),
      '#description' => t('Prevent repeat Activity from the same user for the same entity within this interval'),
      '#options' => $options,
      '#default_value' => $this->options['window'],
    );
  }

 /**
   * Determine if the current Action is valid for this object.
   *
   * @param $eid
   *   The entity id for the activity.
   * @param $actor
   *   The uid of the actor for this activity.
   * @param $timestamp
   *   Unix timestamp when this activity occurred.
   * @param array $objects
   *   The collection of objects for this action.
   * @param mixed $argument1
   *   The first argument passed to the action callback.
   * @param mixed $argument2
   *   The second argument passed to the action callback.
   *
   * @return boolean
   */
  public function valid($eid, $actor, $timestamp, $objects, $argument1, $argument2) {
    if (!empty($this->options['window'])) {
      $count = db_select('activity', 'a')
        ->fields('a', array('aid'))
        ->condition('eid', $eid, '=')
        ->condition('uid', $actor, '=')
        ->condition('created', $timestamp - $this->options['window'], '>')
        ->condition('actions_id', $this->actions_id, '=')
        ->countQuery()
        ->execute()
        ->fetchField();
      return $count == 0;
    }
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
        $current_message = '';
        if (isset($this->templates[$id]) && isset($this->templates[$id][$object_key])) {
          $current_message = $this->templates[$id][$object_key];
        }
        $form[$id][$object_key] = array(
          '#type' => 'textarea',
          '#title' => t($information['title']),
          '#description' => t($information['description']),
          '#default_value' => $current_message,
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
    $languages = array();
    foreach ($this->templates as $language_id => $language_patterns) {
      foreach ($language_patterns as $target_key => $text) {
        $uid = $this->getUid($target_key, $objects);
        if ($uid !== FALSE) {
          $options = array(
            'clear' => TRUE,
            'sanitize' => FALSE,
          );
          if (drupal_multilingual()) {
            if (empty($languages)) {
              $languages = language_list();
            }
            $options['language'] = $languages[$language_id];
          }
          $message = token_replace($text, $objects, $options);
          if (!empty($message)) {
            $messages[$language_id][$uid] = $message;
          }
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
  protected function getUid($key, $objects) {
    if ($key == 'public') {
      return 0;
    }
    elseif ($key == 'current_user') {
      return $GLOBALS['user']->uid;
    }
    if (isset($objects[$key]) && isset($objects[$key]->uid)) {
      return $objects[$key]->uid;
    }

    return FALSE;
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
   * @param array $context
   *  The context array passed into the action callback.
   *
   * @return int / NULL
   */
  abstract public function determineEid($context);

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

  /**
   * Return the timestamp that this Activity happened at.
   *
   * @param array $objects
   *   The objects used in tokenization.
   *
   * @return int
   */
  public function determineTimestamp($objects) {
    return REQUEST_TIME;
  }

  /**
   * Return whether or not the Activity is published.
   *
   * @param array $objects
   *   The objects used in tokenization.
   * @param int $actor
   *   The uid of the actor for this activity.
   */
  public function isPublished($objects, $actor) {
    // Key is to determine if the Actor is not blocked.
    if ($actor == 0) {
      // Anon user is blocked by default and cannot be unblocked. i.e their
      // status can never and will never change, always being 0. Thus, anon
      // gets a pass through.
      return TRUE;
    }

    $status = db_query("SELECT status FROM {users} WHERE uid = :uid", array(":uid" => $actor))->fetchField();
    return $status == 1;
  }

  /**
   * List all eids for this handler, used for batch regeneration and backfilling.
   *
   * @param int $offset
   *   The offset for the query.
   * @param int $limit
   *   The limit for the query.
   */
  public function listEids($offset, $limit) {
    return array('total' => 0, 'arguments' => array());
  }
}

/**
 * Activity handler for node module.
 */
class NodeActivityActionHandler extends ActivityActionHandler {
  /**
   * Return the eid field for this Activity.
   *
   * @param array $context
   *   The context argument passed into the action callback.
   *
   * @return int
   */
  public function determineEid($context) {
    return $context['node']->nid;
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
    if ($this->type == 'node_insert') {
      return $objects['node']->uid;
    }
    return $GLOBALS['user']->uid;
  }

  /**
   * Return the timestamp that this Activity happened at.
   *
   * @param array $objects
   *   The objects used in tokenization.
   *
   * @return int
   */
  public function determineTimestamp($objects) {
    if ($this->type == 'node_insert') {
      return $objects['node']->created;
    }
    return REQUEST_TIME;
  }

  /**
   * Return whether or not the Activity is published.
   *
   * @param array $objects
   *   The objects used in tokenization.
   * @param int $actor
   *   The uid of the actor for this activity.
   */
  public function isPublished($objects, $actor) {
    return parent::isPublished($objects, $actor) && $objects['node']->status;
  }

  /**
   * Load objects for tokenization.
   *
   * @param int $eid
	 *   The entity identifier for this Activity.
   *
   * @return array
   */
  public function loadObjects($eid) {
    $objects['node'] = node_load($eid);

    return $objects;
  }

  /**
   * Return option defaults and structure.
   *
   * @return array.
   */
  public function optionDefinition() {
    $options = parent::optionDefinition();
    $options['types'] = array(
      '#default_value' => array(),
    );
    $options['view_modes'] = array(
      '#default_value' => array(),
    );
    return $options;
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
   * @param $eid
   *   The entity id for the activity.
   * @param $actor
   *   The uid of the actor for this activity.
   * @param $timestamp
   *   Unix timestamp when this activity occurred.
   * @param array $objects
   *   The collection of objects for this action.
   * @param mixed $argument1
   *   The first argument passed to the action callback.
   * @param mixed $argument2
   *   The second argument passed to the action callback.
   *
   * @return boolean
   */
  public function valid($eid, $actor, $timestamp, $objects, $argument1, $argument2) {
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
      $view_modes_check = in_array($argument1, $view_modes);
    }
    return parent::valid($eid, $actor, $timestamp, $objects, $argument1, $argument2) && $type_check && $view_modes_check;
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
      // getUid() method.
      $messages['current_user'] = array(
        'title' => 'Updating User',
        'description' => 'The user who actually updated the node',
      );
    }
    elseif ($this->type == 'node_view') {
      $messages['current_user'] = array(
        'title' => 'Current User',
        'description' => 'The user viewing the node',
      );
    }

    return $messages;
  }

  /**
   * List all eids for this handler, used for batch regeneration and backfilling.
   *
   * @param int $offset
   *   The offset for the query.
   * @param int $limit
   *   The limit for the query.
   */
  public function listEids($offset, $limit) {
    $types = array_filter($this->options['types']);
    // Because the op view currently isn't batchable, it can be skipped.

    $query = db_select('node', 'n')
      ->fields('n', array('nid'), 'eid');
    if (!empty($types)) {
      $query->condition('type', $types, 'IN');
    }

    $count_query = clone $query;
    $total = $count_query->countQuery()->execute()->fetchField();
    $query->range($offset, $limit);

    $arguments = array();
    foreach ($query->execute()->fetchCol() as $eid) {
      $arguments[$eid] = array(
        'argument1' => NULL,
        'argument2' => NULL,
      );
    }
    return array('total' => $total, 'arguments' => $arguments);
  }
}

/**
 * Activity handler for the comment module.
 * Extends the node handler has it presents a lot of the same things, just more.
 */
class CommentActivityActionHandler extends NodeActivityActionHandler {
  /**
   * Load objects for tokenization.
   *
   * @param int $eid
	 *   The entity identifier for this Activity.
   *
   * @return array
   */
  public function loadObjects($eid) {
    $objects = array();
    $objects['comment'] = comment_load($eid);
    $objects['node'] = node_load($objects['comment']->nid);

    return $objects;
  }

  /**
   * Return the eid field for this Activity.
   *
   * @param array $context
   *  The context arguments passed into the action callback.
   *
   * @return int
   */
  public function determineEid($context) {
    return $context['comment']->cid;
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
    return $objects['comment']->uid;
  }

  /**
   * Return the timestamp that this Activity happened at.
   *
   * @param array $objects
   *   The objects used in tokenization.
   *
   * @return int
   */
  public function determineTimestamp($objects) {
    return $objects['comment']->created;
  }

  /**
   * Return whether or not the Activity is published.
   *
   * @param array $objects
   *   The objects used in tokenization.
   * @param int $actor
   *   The actor for the activity.
   */
  public function isPublished($objects, $actor) {
    return parent::isPublished($objects, $actor) && $objects['comment']->status;
  }

   /**
   * Return an array of message types.
   */
  protected function messages() {
    $messages = parent::messages();
    $messages['comment'] = array(
      'title' => 'Comment Author',
      'description' => 'The author of the comment',
    );
    $messages['node_comment'] = array(
      'title' => 'Comment and Node Author',
      'description' => 'When both the comment and the node author are the same display this message',
    );
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
  protected function getUid($key, $objects) {
    if ($key == 'public') {
      return 0;
    }
    elseif ($key == 'current_user') {
      return $GLOBALS['user']->uid;
    }
    // node_comment key is when the comment and the node author are the same.
    elseif ($key == 'node_comment' && $objects['node']->uid == $objects['comment']->uid) {
      return $objects['comment']->uid;
    }
    if (isset($objects[$key]) && isset($objects[$key]->uid)) {
      return $objects[$key]->uid;
    }

    return FALSE;
  }

  /**
   * List all eids for this handler, used for batch regeneration and backfilling.
   *
   * @param int $offset
   *   The offset for the query.
   * @param int $limit
   *   The limit for the query.
   */
  public function listEids($offset, $limit) {
    $types = array_filter($this->options['types']);

    $query = db_select('comment', 'c')
      ->fields('c', array('cid'), 'eid');
    if (!empty($types)) {
      $node_alias = $query->join('node', 'n', 'n.nid = c.nid');
      $query->condition($node_alias . '.type', $types, 'IN');
    }

    $count_query = clone $query;
    $total = $count_query->countQuery()->execute()->fetchField();

    $query->range($offset, $limit);
    $arguments = array();
    foreach ($query->execute()->fetchCol() as $eid) {
      $arguments[$eid] = array(
        'argument1' => NULL,
        'argument2' => NULL,
      );
    }
    return array('total' => $total, 'arguments' => $arguments);
  }
}

/**
 * Activity Action Handler for the user triggers.
 */
class UserActivityActionHandler extends ActivityActionHandler {
  /**
   * Return an array of message types.
   */
  protected function messages() {
    $messages = parent::messages();
    if ($this->type == 'user_update') {
      $messages['current_user'] = array(
        'title' => 'Updating User',
        'description' => 'The user doing the updating',
      );
    }
    $messages['user'] = array(
      'title' => 'User Account',
      'description' => 'User account that the action is applied to',
    );
    return $messages;
  }
  /**
   * Load objects for tokenization.
   *
   * @param int $eid
   *   The entity identifier for this Activity.
   *
   * @return array
   */
  public function loadObjects($eid) {
    return array(
      'user' => user_load($eid),
    );
  }

  /**
   * Return the eid field for this Activity.
   *
   * @param array $context
   *  The context argument passed into the action callback.
   *
   * @return int
   */
  public function determineEid($context) {
    return $context['account']->uid;
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
    if ($this->type == 'user_update') {
      return $GLOBALS['user']->uid;
    }
    return $objects['user']->uid;
  }

  /**
   * Return the timestamp that this Activity happened at.
   *
   * @param array $objects
   *   The objects used in tokenization.
   *
   * @return int
   */
  public function determineTimestamp($objects) {
    if ($this->type == 'user_insert') {
      return $objects['user']->created;
    }
    return REQUEST_TIME;
  }

  /**
   * Return whether or not the Activity is published.
   *
   * @param array $objects
   *   The objects used in tokenization.
   * @param int $actor
   *   The actor for this activity.
   */
  public function isPublished($objects, $actor) {
    $user_active = $objects['user']->status == 1;
    if ($this->type != 'user_insert') {
      return $user_active && parent::isPublished($objects, $actor);
    }
    return $user_active;
  }

  /**
   * Return option defaults and structure.
   *
   * @return array.
   */
  public function optionDefinition() {
    $options = parent::optionDefinition();

    $options['roles'] = array(
      '#default_value' => array(),
    );
    return $options;
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
    $roles = user_roles();
    $form['roles'] = array(
      '#type' => 'checkboxes',
      '#title' => t('Roles'),
      '#options' => $roles,
      '#default_value' => $this->options['roles'],
    );
  }

  /**
   * Determine if the current Action is valid for this object.
   *
   * @param $eid
   *   The entity id for the activity.
   * @param $actor
   *   The uid of the actor for this activity.
   * @param $timestamp
   *   Unix timestamp when this activity occurred.
   * @param array $objects
   *   The collection of objects for this action.
   * @param mixed $argument1
   *   The first argument passed to the action callback.
   * @param mixed $argument2
   *   The second argument passed to the action callback.
   *
   * @return boolean
   */
  public function valid($eid, $actor, $timestamp, $objects, $argument1, $argument2) {
    $roles = array_filter($this->options['roles']);
    if (!empty($roles)) {
      $user_roles = array_keys($objects['user']->roles);
      $intersect = array_intersect($user_roles, $roles);
      return parent::valid($eid, $actor, $timestamp, $objects, $argument1, $argument2) && !empty($intersect);
    }
    return parent::valid($eid, $actor, $timestamp, $objects, $argument1, $argument2);
  }

  /**
   * List all eids for this handler, used for batch regeneration and backfilling.
   *
   * @param int $offset
   *   The offset for the query.
   * @param int $limit
   *   The limit for the query.
   */
  public function listEids($offset, $limit) {
    $roles = array_filter($this->options['roles']);

    $query = db_select('users', 'u')
      ->fields('u', array('uid'), 'eid')
      ->condition('u.uid', 0, '<>')
      ->range($offset, $limit);
    if (!empty($roles)) {
      $roles_alias = $query->join('users_roles', 'r', 'r.uid = u.uid');
      $query->condition($roles_alias . '.rid', $roles, 'IN');
    }

    $count_query = clone $query;
    $total = $count_query->countQuery()->execute()->fetchField();

    $arguments = array();
    foreach ($query->execute()->fetchCol() as $eid) {
      $arguments[$eid] = array(
        'argument1' => NULL,
        'argument2' => NULL,
      );
    }
    return array('total' => $total, 'arguments' => $arguments);
  }
}

/**
 * @} End of "ingroup activity handlers".
 */