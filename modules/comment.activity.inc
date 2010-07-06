<?php
// $Id: $

/**
 * @file:
 * Provides Activity access grant implementation for comment module.
 */

/**
 * Implements hook_activity_grants().
 */
function comment_activity_grants($activity) {
  $grants = array();
  if (!empty($activity->nid)) {
    $grants['comment'] = array(
      $activity->nid,
    );
  }
  return $grants;
}

/**
 * Implements hook_activity_access_grants().
 */
function comment_activity_access_grants($account) {
  $grants = array();
  $nodes_commented_on = db_query("SELECT nid FROM {comment} WHERE uid = :uid", array(":uid" => $account->uid))->fetchCol();
  if (!empty($nodes_commented_on)) {
    $grants['comment'] = $nodes_commented_on;
  }

  return $grants;
}