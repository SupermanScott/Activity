<?php
// $Id: activity.admin.inc,v 1.1.2.41 2010/05/25 04:39:33 scottreynolds Exp $

/**
 * @file activity.admin.inc
 * Contains administrative forms for activity.module
 */

/**
 * Menu callback. Provides checkboxes for a user's activity feed.
 */
function activity_user_settings(&$form_state, $account) {
  if (!isset($account->activity_ignore)) {
    $account->activity_ignore = array();
  }
  $form['actions'] = array(
    '#type' => 'fieldset',
    '#title' => t('Actions within !site', array('!site' => variable_get('site_name', 'drupal'))),
    '#description' => t('By default, all of the following actions you perform can be viewed on this site. <br />Select the activities you would like to <strong>exclude</strong> people from seeing.'),
  );
  $form['actions']['activity_ignore']['#tree'] = TRUE;

  // In order to see what actions are being implemented on a given system we
  // need to query the actions table joining the trigger_assignments.
  $query = "SELECT a.aid, a.description, ta.hook, ta.op FROM {actions} a
            INNER JOIN {trigger_assignments} ta ON ta.aid = a.aid
            WHERE a.type = 'activity'";
  $result = db_query($query);
  while ($row = db_fetch_object($result)) {
    $module = activity_module_name($row->hook);

    // The module might be uninstalled and therefore, not exist anymore.
    if (!empty($module)) {
      $description = $row->description;
      // This means we're basically keying off of the aid and not the triggers. We
      // could possible key off of the triggers instead, and give the user more
      // fine grained control over (i.e. I want to record my own node inserts but
      // not my node updates).
      $form['actions']['activity_ignore'][$row->aid] = array(
        '#type' => 'radios',
        '#title' => $description,
        '#options' => array(1 => t('Record an activity message'), 0 => t('Do not record')),
        '#default_value' => isset($account->activity_ignore[$row->aid]) ? $account->activity_ignore[$row->aid] : 1,
      );
    }
  }
  $form['submit'] = array(
    '#type' => 'submit',
     '#value' => t('Save'),
   );
  $form['#account'] = $account;
   return $form;
}

/**
 * User settings form submit handler.
 */
function activity_user_settings_submit($form, &$form_state) {
  user_save($form['#account'], array('activity_ignore' => $form_state['values']['activity_ignore']));
}

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
  $results = db_query($sql, array(), array(
               'target' => 'slave',
               'fetch' => PDO::FETCH_ASSOC))->fetchAll();
  if (!empty($results)) {
    return theme('activity_settings_actions_list', array('results' => $results));
  }

  return t('There are no Activity Templates created yet. !link', array('!link' => l(t('Create one now.'), 'admin/structure/activity/create')));
}

/**
 * Form builder to select what type of Activity to record.
 */
function activity_create_form($form, &$form_state) {
  $hooks = activity_cache_get('hooks');
  foreach ($hooks as $hook => $information) {
    $hook_options[$hook] = t($information['name']);
  }

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
  actions_save('activity_record', 'activity', array('aid' => $aid, 'templates' => array(), 'options' => array()), $form_state['values']['label'], $aid);

  // Write to the trigger assignments.
  $record = array(
    'hook' => $form_state['values']['hook'],
    'weight' => 0,
    'aid' => $aid
  );
  drupal_write_record('trigger_assignments', $record);
  $form_state['redirect'] = 'admin/structure/activity/configure/' . $aid;
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
        '#size' => 5,
      ),
      'argument2' => array(
        '#type' => 'textfield',
        '#title' => t('Argument 2'),
        '#size' => 5,
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
    '#access' => $action->batch,
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

  if ($handler->valid($objects, $form_state['values']['test']['additional_arguments']['argument1'], $form_state['values']['test']['additional_arguments']['argument2'])) {
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
      'file' => drupal_get_path('module', 'activity') . '/activity.admin.inc',
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
function activity_actions_delete_form(&$form_state, $action) {
  $form['aid'] = array(
    '#type' => 'hidden',
    '#value' => $action->aid,
  );
  return confirm_form($form,
    t('Are you sure you want to delete the action %action?', array('%action' => $action->description)),
    'admin/build/activity',
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
  watchdog('user', 'Deleted action %aid (%action)', array('%aid' => $aid, '%action' => $action->description));
  drupal_set_message(t('Action %action was deleted', array('%action' => $description)));
  $form_state['redirect'] = 'admin/build/activity';
}

/**
 * Form builder to dispaly settings for activity module
 */
function activity_settings_form(&$form_state = NULL) {
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
function activity_delete_confirm(&$form_state, $aid) {
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
    activity_delete($form_state['values']['aid']);
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
      array('activity_access_rebuild_process', array()),
    ),
    'file' => drupal_get_path('module', 'activity') . '/activity.admin.inc',
  );
  batch_set($batch);
  $form_state['redirect'] = 'admin/config/content/activity';
}

/**
 * Batch API processing operation. Rebuilding Access table.
 *
 * @param array $context
 *  The batch api context array.
 *
 * @return none
 */
function activity_access_rebuild_process(&$context) {
  if (!isset($context['sandbox']['last_aid'])) {
    // Set up the sandbox for the first time.
    $context['sandbox']['last_aid'] = 0;
    $context['sandbox']['progress'] = 0;

    // Activity can be happening on the site. Any Activity happening after this point
    // will not be rebuilt. This is ok, as in that case, the new Activity will receive
    // any and all new Activity Access Realms.
    $context['sandbox']['max'] = db_query("SELECT COUNT(aid) FROM {activity}")->fetchField();
  }
  // Process 100 Activities at a time.
  $limit = 100;

  $activities = db_select('activity', 'a')
    ->fields('a')
    ->condition('aid', $context['sandbox']['last_aid'], '>')
    ->range(0, $limit)
    ->execute()
    ->fetchAll();

  foreach ($activities as $activity) {
    $grants = activity_get_grants($activity);
    // Delete existing records.
    db_delete('activity_access')
      ->condition('aid', $activity->aid)
      ->execute();

    // Insert new ones.
    foreach ($grants as $realm => $values) {
      foreach ($values as $value) {
        $perm = new stdClass();
        $perm->aid = $activity->aid;
        $perm->realm = $realm;
        $perm->value = $value;
        drupal_write_record('activity_access', $perm);
      }
    }

    // Update sandbox variables.
    $context['sandbox']['last_aid'] = $activity->aid;
    $context['sandbox']['progress']++;
  }
  // Check if not finished.
  if ($context['sandbox']['progress'] < $context['sandbox']['max']) {
    $context['finished'] = $context['sandbox']['progress'] / $context['sandbox']['max'];
  }
  else {
    // If finished, delete the sandbox.
    unset($context['sandbox']);
  } 
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
      array('activity_recreate_messages_step', array($action->actions_id)),
    ),
    'file' => drupal_get_path('module', 'activity') . '/activity.admin.inc',
  );
  batch_set($batch);
  batch_process('admin/structure/activity');
}

/**
 * Batch operation callback to generate new messages.
 *
 * @param $action_id
 *   The {actions}.aid to generate messages for.
 * @param array $context
 *   The context of this batch operation.
 */
function activity_recreate_messages_step($action_id, &$context) {
  if (!isset($context['sandbox']['last_aid'])) {
    $context['sandbox'] = array(
      'last_aid' => 0,
      'progress' => 0,
      'max' => db_query("SELECT COUNT(aid) FROM {activity} WHERE actions_id = :action_id", array(":action_id" => $action_id))->fetchField(),
    );
  }
  $limit = 50;

  $records = db_select('activity', 'a')
    ->fields('a')
    ->condition('a.actions_id', $action_id)
    ->condition('a.aid', $context['sandbox']['last_aid'], '>')
    ->range(0, $limit)
    ->execute()
    ->fetchAll();
  $context['sandbox']['progress'] += count($records);
  activity_recreate_messages($records);

  if ($context['sandbox']['progress'] < $context['sandbox']['max']) {
    $context['finished'] = $context['sandbox']['progress'] / $context['sandbox']['max'];
  }
  else {
    unset($context['sandbox']);
  }
}