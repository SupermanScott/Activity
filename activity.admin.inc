<?php
// $Id: $

/**
 * @file activity.admin.inc
 * Contains administrative forms for activity.module
 */

/**
 * Menu callback to display all the currently setup Activities.
 */
function activity_admin_overview() {
  $results = array();
  $sql = "SELECT ta.hook, ta.aid, a.label
          FROM {trigger_assignments} ta
          INNER JOIN {actions} a
            ON a.aid = ta.aid
          WHERE a.type = 'activity'";
  $results = db_query($sql)->fetchAll();
  if (!empty($results)) {
    return theme('activity_settings_actions_list', array('results' => $results));
  }

  return t('There are no Activity Templates created yet. !link', array('!link' => l(t('Create one now.'), 'admin/structure/activity/create')));
}

/**
 * Form builder to select what type of Activity to record.
 */
function activity_create_form($form, &$form_state) {
  $hook_options = activity_form_options_hooks();

  $form['label'] = array(
    '#type' => 'textfield',
    '#default_value' => '',
    '#title' => t('Label'),
    '#weight' => -10,
    '#required' => TRUE,
  );

  $form['hook'] = array(
    '#title' => t('Choose your hook'),
    '#description' => t('The hook that this Activity will record messages.'),
    '#options' => $hook_options,
    '#type' => 'radios',
    '#required' => TRUE,
  );

  $form['next'] = array(
    '#type' => 'submit',
    '#value' => t('Create'),
  );
  return $form;
}

/**
 * Form submit callback to create the new ActivityActionHandler.
 */
function activity_create_form_submit($form, &$form_state) {
  // Save it once to get the aid.
  $aid = actions_save('activity_record', 'activity', array(), $form_state['values']['label']);

  // Now save the aid away to the parameters.
  $handler = activity_load_handler($form_state['values']['hook']);
  actions_save('activity_record', 'activity', array('aid' => $aid, 'templates' => array(), 'options' => ActivityActionHandler::defaultOptions($handler->optionDefinition())), $form_state['values']['label'], $aid);

  // Write to the trigger assignments.
  $record = array(
    'hook' => $form_state['values']['hook'],
    'weight' => 0,
    'aid' => $aid
  );
  drupal_write_record('trigger_assignments', $record);
  $form_state['redirect'] = array('admin/structure/activity/configure/' . $aid, array('query' => array('new' => 1)));
}

/**
 * Form builder for the Handler configuration.
 */
function activity_configure_handler_form($form, $form_state, ActivityActionHandler $action) {
  $form += array(
    '#tree' => TRUE,
    'options' => array(),
    'messages' => array(),
    'test' => array(),
    'label' => array(
      '#type' => 'textfield',
      '#default_value' => $action->label,
      '#title' => t('Label'),
      '#weight' => -10,
    ),
  );

  $form_state += array(
    'messages' => array(
      'en' => array(
        0 => 'Enter an Eid to test this message'
      ),
    ),
  );

  // Add in the handlers options and messages.
  $action->optionForm($form['options'], $form_state);
  $action->messagesForm($form['messages'], $form_state);

  $table_id = 'activity-test-table';
  $form['#actions_id'] = $action->actions_id;
  $form['test'] = array(
    '#prefix' => '<div class="container-inline">',
    '#suffix' => '</div>',
    'eid' => array(
      '#type' => 'textfield',
      '#title' => t('Enter the ID'),
      '#default_value' => '',
      '#size' => 5,
    ),
    'additional_arguments' => array(
      'argument1' => array(
        '#type' => 'textfield',
        '#title' => t('Argument 1'),
        '#size' => 25,
      ),
      'argument2' => array(
        '#type' => 'textfield',
        '#title' => t('Argument 2'),
        '#size' => 25,
      ),
    ),
    'submit_test' => array(
      '#type' => 'submit',
      '#value' => t('Test'),
      '#submit' => array('activity_message_test_submit'),
      '#ajax' => array(
        'callback' => 'activity_message_test_ajax_callback',
        'wrapper' => $table_id,
      ),
    ),
  );

  $rows = array();
  foreach ($form_state['messages'] as $lang => $messages) {
    foreach ($messages as $uid => $message) {
      $rows[] = array($lang, $uid, $message);
    }
  }
  $form['message_results'] = array(
    '#markup' => theme('table', array('header' => array(t('Language'), t('User id'), t('Message')),'rows' => $rows, 'attributes' => array('id' => $table_id))),
  );
  $form['save'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );
  $form['save_update'] = array(
    '#type' => 'submit',
    '#value' => t('Save and Update Existing'),
    '#access' => $action->batch && empty($_GET['new']),
  );
  $form['save_create'] = array(
    '#type' => 'submit',
    '#value' => t('Save and Create'),
    '#access' => $action->batch && !empty($_GET['new']),
  );
  return $form;
}

/**
 * Submit handler for the test button.
 */
function activity_message_test_submit($form, &$form_state) {
  $form_state['messages'] = array(
    'en' => array(
      0 => 'Failed Validation',
    ),
  );
  $handler = activity_handler_load($form['#actions_id']);
  $handler->options = $form_state['values']['options'];
  $handler->templates = $form_state['values']['messages'];

  $objects = $handler->loadObjects($form_state['values']['test']['eid']);
  drupal_alter('activity_objects', $objects, $handler->type);

  if ($handler->valid($form_state['values']['test']['eid'], $handler->determineActor($objects), REQUEST_TIME, $objects, $form_state['values']['test']['additional_arguments']['argument1'], $form_state['values']['test']['additional_arguments']['argument2'])) {
    $form_state['messages'] = $handler->tokenize($objects);
  }

  $form_state['rebuild'] = TRUE;
}

/**
 * AJAX callback for the test ajax button.
 */
function activity_message_test_ajax_callback($form, $form_state) {
  return $form['message_results'];
}

/**
 * Form submit handler for the configuration form.
 */
function activity_configure_handler_form_submit($form, &$form_state) {
  $params = array(
    'aid' => $form['#actions_id'],
    'options' => $form_state['values']['options'],
    'templates' => $form_state['values']['messages'],
  );
  actions_save('activity_record', 'activity', $params, $form_state['values']['label'], $form['#actions_id']);

  if ($form_state['values']['op'] == t('Save and Update Existing')) {
    $batch = array(
      'title' => t('Regenerating @label messages', array('@label' => $form_state['values']['label'])),
      'operations' => array(
        array('activity_recreate_messages_step', array($form['#actions_id'])),
      ),
      'file' => drupal_get_path('module', 'activity') . '/activity.batch.inc',
    );
    batch_set($batch);
  }
  elseif ($form_state['values']['op'] == t('Save and Create')) {
    $batch = array(
      'title' => t('Generating @label messages', array('@label' => $form_state['values']['label'])),
      'operations' => array(
        array('activity_batch_regenerate_step', array($form['#actions_id'])),
      ),
      'file' => drupal_get_path('module', 'activity') . '/activity.batch.inc',
    );
    batch_set($batch);
  }
}

/**
 * Create the form for confirmation of deleting an activity action.
 *
 * @ingroup forms
 * @see activity_actions_delete_form_submit()
 */
function activity_actions_delete_form($form, &$form_state, $action) {
  $form['aid'] = array(
    '#type' => 'value',
    '#value' => $action->aid,
  );
  $form['delete'] = array(
    '#type' => 'checkbox',
    '#title' => t('Delete all activities'),
    '#description' => t('Delete all activities associated with this template'),
    '#default_value' => TRUE,
  );
  return confirm_form($form,
    t('Are you sure you want to delete the action %action?', array('%action' => $action->label)),
    'admin/structure/activity',
    t('This cannot be undone.'),
    t('Delete'), t('Cancel')
  );
}

/**
 * Process activity_actions_delete form submissions.
 *
 * Post-deletion operations for activity action deletion.
 */
function activity_actions_delete_form_submit($form, &$form_state) {
  $aid = $form_state['values']['aid'];
  $action = actions_load($aid);
  actions_delete($aid);

  if ($form_state['values']['delete']) {
    $batch = array(
      'title' => t('Deleting activities'),
      'operations' => array(
        array('activity_batch_delete', array($aid))
      ),
      'file' => drupal_get_path('module', 'activity') . '/activity.batch.inc',
    );
    batch_set($batch);
  }

  watchdog('activity', 'Deleted action %aid (%action)', array('%aid' => $aid, '%action' => $action->label));
  drupal_set_message(t('Action %action was deleted', array('%action' => $action->label)));
  $form_state['redirect'] = 'admin/structure/activity';
}

/**
 * Form builder to dispaly settings for activity module
 */
function activity_settings_form($form, &$form_state = NULL) {
  $form['activity_expiration'] = array(
    '#type' => 'fieldset',
    '#title' => t('Activity Expiration Settings'),
    '#element_validate' => array('activity_expire_validate'),
  );
  
  $form['activity_expiration']['activity_expire'] = array(
    '#type' => 'select',
    '#title' => t('Activity log purge'),
    '#description' => t("Allows you to set a time limit for storing activity records. Select 0 to keep all activity records."),
    '#options' => drupal_map_assoc(array(0, 3600, 7200, 14400, 21600, 43200, 86400, 604800, 1209600, 2419200, 7257600, 15724800, 31536000), 'format_interval'),
    '#default_value' => variable_get('activity_expire', 0),
  );
  
  $form['activity_expiration']['activity_min_count'] = array(
    '#type' => 'select',
    '#title' => t('Minimum Activities'),
    '#description' => t('This is the minimum number activities that the user must have created before deleting any old activities.'),
    '#options' => drupal_map_assoc(range(0, 200, 10)),
    '#default_value' => variable_get('activity_min_count', 0),
  );
  

  // Allow realms provided by modules.
  $realms = array();
  foreach (activity_cache_get('all_realms') as $realm => $information) {
    $realms[$realm] = $information['name'];
  }

  // Set up the default value for this set of checkboxes.
  $enabled = array();
  foreach (activity_cache_get('realms') as $realm => $information) {
    $enabled[$realm] = $realm;
  }

  $form['activity_access'] = array(
    '#type' => 'fieldset',
    '#title' => t('Activity Access Control'),
    '#attributes' => array('id' => 'activity-access-fieldset'),
  );
  $form['activity_access']['activity_access_realms'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Realms'),
    '#description' => t('Select the realms for which Activity access records should be recorded. These realms will allow a View to filter in more then just one users Activity.'),
    '#options' => $realms,
    '#default_value' => $enabled,
  );

  $form['activity_access']['activity_access_rebuild'] = array(
    '#type' => 'submit',
    '#value' => t('Rebuild Activity Access Table'),
    '#submit' => array('activity_access_batch_set'),
  );

  // This tells system_settings_form to use array_filter for the checkboxes.
  $form['array_filter'] = array('#type' => 'value', '#value' => TRUE);

  return system_settings_form($form);
}

/**
 * Element validate callback
 */
function activity_expire_validate($element, &$form_state) {
  if (empty($form_state['values']['activity_expire']) && !empty($form_state['values']['activity_min_count'])) {
    form_set_error('activity_expire', t('You must set a time limit in order to use the minimum count'));
  }
}

/**
 * Menu callback -- ask for confirmation of activity deletion
 */
function activity_delete_confirm($form, $form_state, $aid) {
  $form['aid'] = array(
    '#type' => 'value',
    '#value' => $aid,
  );

  return confirm_form($form,
    t('Are you sure you want to delete this activity?'),
    $_GET['destination'],
    t('This action cannot be undone.'),
    t('Delete'),
    t('Cancel')
  );
}

/**
 * Execute activity deletion
 */
function activity_delete_confirm_submit($form, &$form_state) {
  if ($form_state['values']['confirm']) {
    activity_delete(array($form_state['values']['aid']));
  }

  $form_state['redirect'] = '<front>';
}

/**
 * FAPI form submit function for the activity_rebuild button.
 *
 * @param array $form
 *  The FAPI form structure array.
 *
 * @param array &$form_state
 *  The FAPI form state array.
 *
 * @return none
 */
function activity_access_batch_set($form, &$form_state) {
  // Address usability concern where the realms arn't set.
  $submitted_realms = array_filter($form_state['values']['activity_access_realms']);
  variable_set('activity_access_realms', $submitted_realms);

  $batch = array(
    'title' => t('Rebuilding Activity Access Table'),
    'operations' => array(
      array('activity_batch_access_rebuild_process', array()),
    ),
    'file' => drupal_get_path('module', 'activity') . '/activity.batch.inc',
  );
  batch_set($batch);
  $form_state['redirect'] = 'admin/config/content/activity';
}

/**
 * Page callback to set up a batch process.
 *
 * @param $action
 *  ActivityActionsHandler to recreate messages for.
 */
function activity_admin_recreate(ActivityActionHandler $action) {
  $batch = array(
    'title' => t('Regenerating @label messages', array('@label' => $action->label)),
    'operations' => array(
      array('activity_batch_recreate_messages_step', array($action->actions_id)),
    ),
    'file' => drupal_get_path('module', 'activity') . '/activity.batch.inc',
  );
  batch_set($batch);
  batch_process('admin/structure/activity');
}


/**
 * Set a batch process to regenerate activity for a specific hook and op pair.
 *
 * @param $action
 *  ActivityActionHandler to create messages for.
 *
 * @return none
 */
function activity_batch_regenerate(ActivityActionHandler $action) {
  $batch = array(
    'title' => t('Creating @label messages', array('@label' => $action->label)),
    'operations' => array(
      array('activity_batch_delete', array($action->actions_id)),
      array('activity_batch_regenerate_step', array($action->actions_id))
    ),
    'file' => drupal_get_path('module', 'activity') . '/activity.batch.inc',
  );
  batch_set($batch);
  batch_process('admin/structure/activity');
}
