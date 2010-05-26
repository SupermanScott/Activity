<?php
// $Id: $

/**
 * @file
 * Provides Activity integration for the node module.
 */
/**
 * Implementation of hook_activity_grants().
 */
function node_activity_grants($activity) {
  $grants = array();
  if (!empty($activity->nid)) {
    $grants = array(
      'node_author' => array(
        $activity->nid
      ),
    );
  }
  return $grants;
}

/**
 * Implementation of hook_activity_access_grants().
 */
function node_activity_access_grants($account) {
  $grants = array();

  // Select the nodes you have created.
  $created_result = db_query("SELECT nid FROM {node} WHERE uid = :uid", array(':uid' => $account->uid))->fetchCol();
  if (!empty($created_result)) {
    $grants['node_author'] = $created_result;
  }

  return $grants;
}
