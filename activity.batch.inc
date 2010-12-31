<?php
// $Id: $
/**
 * @file:
 * Contains Batch API functions for long running processing.
 */
/**
 * Batch API processing operation. Rebuilding Access table.
 *
 * @param array $context
 *  The batch api context array.
 *
 * @return none
 */
function activity_batch_access_rebuild_process(&$context) {
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
 * Batch operation callback to generate new messages.
 *
 * @param $action_id
 *   The {actions}.aid to generate messages for.
 * @param array $context
 *   The context of this batch operation.
 */
function activity_batch_recreate_messages_step($action_id, &$context) {
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

/**
 * Batch deletion step.
 *
 * @param $aid
 *  The actions.aid for this template.
 * @param $batch_context
 *  The context array for this batch operation.
 */
function activity_batch_delete($aid, &$batch_context) {
  if (!isset($batch_context['sandbox']['last_activity_id'])) {
    $batch_context['sandbox']['last_activity_id'] = 0;
    $batch_context['sandbox']['progress'] = 0;
    $batch_context['sandbox']['max'] = db_query("SELECT COUNT(aid) FROM {activity} WHERE actions_id = :aid", array(':aid' => $aid))->fetchField();
  }
  $limit = 200;
  $activity_to_be_deleted = db_select('activity', 'a')
    ->fields('a', array('aid'))
    ->condition('a.aid', $batch_context['sandbox']['last_activity_id'], '>')
    ->condition('a.actions_id', $aid)
    ->orderBy('a.aid', 'ASC')
    ->range(0, $limit)
    ->execute()
    ->fetchAll();
  $activity_ids = array();
  if (!empty($activity_to_be_deleted)) {
    foreach ($activity_to_be_deleted as $row) {
      $activity_ids[] = $row->aid;
      $batch_context['sandbox']['last_activity_id'] = $row->aid;
      $batch_context['sandbox']['progress']++;
    }
    activity_delete($activity_ids);
  }

  // Check if not finished.
  if (!empty($activity_to_be_deleted) && $batch_context['sandbox']['progress'] < $batch_context['sandbox']['max']) {
    $batch_context['finished'] = $batch_context['sandbox']['progress'] / $batch_context['sandbox']['max'];
  }
  else {
    // If finished, delete the sandbox.
    unset($batch_context['sandbox']);
  }
  
}

/**
 * Batch regeneration step.
 *
 * @param $aid
 *  The actions.aid for this template.
 * @param $batch_context
 *  An array representing the context for the batch operation.
 */
function activity_batch_regenerate_step($aid, &$batch_context) {
  $handler = activity_handler_batchable_load($aid);
  if (empty($handler)) {
    // Return error? Done?
  }
  $limit = 50;

  // Set up the sandbox for the regeneration.
  if (!isset($batch_context['sandbox']['progress'])) {
    $batch_context['sandbox']['progress'] = 0;
    $batch_context['sandbox']['last_eid'] = 0;
  }
  // Extracts $arguments and $total.
  extract($handler->listEids($batch_context['sandbox']['progress'], $limit));

  foreach ($arguments as $eid => $arguments) {
    // Ensure they are there.
    $arguments += array('argument1' => NULL, 'argument2' => NULL);
    activity_save_activity($handler, $eid, $arguments['argument1'], $arguments['argument2']);
    $batch_context['sandbox']['progress']++;
  }
  // Check if not finished.
  if ($batch_context['sandbox']['progress'] < $total) {
    $batch_context['finished'] = $batch_context['sandbox']['progress'] / $batch_context['sandbox']['max'];
  }
  else {
    // If finished, delete the sandbox.
    unset($batch_context['sandbox']);
  }
}