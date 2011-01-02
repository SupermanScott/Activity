<?php
// $Id: $

/**
 * @file
 * Builds placeholder replacement tokens for activity data.
 */

/**
 * Implements hook_token_info().
 */
function activity_token_info() {
  return array(
    'tokens' => array(
      'user' => array(
        'activity-profile-link' => array(
          'name' => t("Activity Profile Link"),
          'description' => t("Activity-themed profile link"),
        ),
      ),
      'node' => array(
        'activity-node-link' => array(
          'name' => t("Activity Node Link"),
          'description' => t("Link to the node using the node's title"),
        ),
      ),
    ),
  );
}

/**
 * Implements hook_tokens().
 */
function activity_tokens($type, $tokens, array $data = array(), array $options = array()) {
  $replacements = array();

  if ($type == 'user' && !empty($data['user'])) {
    foreach ($tokens as $name => $original) {
      switch ($name) {
        case 'activity-profile-link':
          $replacements[$original] = theme('activity_username', array('account' => $data['user']));
          break;
      }
    }
  }

  if ($type == 'node' && !empty($data['node'])) {
    foreach ($tokens as $name => $original) {
      switch ($name) {
        case 'activity-node-link':
          $replacements[$original] = l($data['node']->title, 'node/' . $data['node']->nid);
          break;
      }
    }
  }

  return $replacements;
}
