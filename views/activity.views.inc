<?php
// $Id: activity.views.inc,v 1.1.2.22 2010/04/26 17:07:59 scottreynolds Exp $
/**
 * @file: provides views data integration for activity module as the base table
 */

/**
 * Implementation of hook_views_data().
 */
function activity_views_data() {
  $data['activity']['table']['group'] = t('Activity');
  
  $data['activity']['table']['base'] = array(
    'field' => 'aid',
    'title' => t('Activity'),
    'help' => t('Activity are items that are recorded for a users activity within the site'),
  );
  
  // Database columns
  $data['activity']['aid'] = array(
    'title' => t('Activity Unique ID'),
    'help' => t('The unique id for each activity'),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
  );
  $data['activity']['uid'] = array(
    'title' => t('Activity Actor'),
    'help' => t('The ID of the person who recorded the activity'),
    'argument' => array(
      'handler' => 'views_handler_argument_user_uid',
    ),
    'filter' => array(
      'title' => t('Name'),
      'handler' => 'views_handler_filter_user_name',
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
  );
  $data['activity']['type'] = array(
    'title' => t('Type'),
    'help' => t('The hook that generated the Activity'),
    'field' => array(
      'handler' => 'activity_views_handler_field_type',
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_in_operator',
      'options callback' => 'activity_form_options_hooks',
    ),
    'argument' => array(
      'handler' => 'views_handler_argument_string',
    ),
  );
  $data['activity']['created'] = array(
    'title' => t('Activity Creation Date'),
    'help' => t('The time that the activity was created'),
    'field' => array(
      'handler' => 'views_handler_field_date',
      'click sortable' => TRUE,
    ),
    'sort' => array(
      'handler' => 'views_handler_sort_date',
    ),
  );

  $data['activity']['status'] = array(
    'title' => t('Published'),
    'help' => t('Whether or not the activity is published'),
    'field' => array(
      'handler' => 'views_handler_field_boolean',
      'click sortable' => TRUE,
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_boolean_operator',
      'label' => t('Published'),
      'type' => 'yes-no',
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
  );

  $data['activity']['actions_id'] = array(
    'title' => t('Template'),
    'help' => t('The template that produced the message'),
    'filter' => array(
      'handler' => 'activity_views_handler_filter_actions_id',
    ),
  );
  
  $data['activity_access']['table']['group'] = t('Activity Access');
  $data['activity_access']['table']['join'] = array(
    'activity' => array(
      // We cannot guarantee that there is activity_access records.
      'type' => 'LEFT',
      'left_field' => 'aid',
      'field' => 'aid',
    ),
  );

  // Create a fake table that tells Views how to join to the actions table for
  // the label.
  $data['activity_actions']['table'] = array(
    'group' => t('Activity'),
    'join' => array(
      'activity' => array(
        'type' => 'INNER',
        'field' => 'aid',
        'left_field' => 'actions_id',
        'table' => 'actions',
      ),
    ),
  );
  $data['activity_actions']['label'] = array(
    'title' => t('Activity Label'),
    'help' => t('Label assigned by the admin to this template'),
    'field' => array(
      'handler' => 'views_handler_field',
    ),
  );
  
  // Sets the context accound for all access filters going forward
  $data['activity_access']['context_user'] = array(
    'title' => t('Activity Context User'),
    'help' => t('This sets Activity User context so all the access filters can key off of it'),
    'argument' => array(
      'handler' => 'activity_views_handler_argument_activity_user',
    ),
  );
    
  $data['activity_access']['access_filter'] = array(
    'title' => t('Access Filters'),
    'help' => t('Applies Activity Access control based on the selected realms for the current user'),
    'filter' => array(
      'handler' => 'activity_views_handler_filter_access',
      'options callback' => 'activity_realm_options',
    ),
  );
  
  // describe the targets table so Views knows how to join it in
  // this table will be used to join the activity table to the message table
  $data['activity_targets']['table'] = array(
    'group' => t('Activity'),
    'join' => array(
      'activity' => array(
        'type' => 'INNER',
        'left_field' => 'aid',
        'field' => 'aid',
        'extra' => array(
          array(
            'value' => 0, // grab the non-personal message
            'numeric' => TRUE,
            'field' => 'uid',
          ),
          array(
            'value' => '***CURRENT_LANGUAGE***',
            'field' => 'language',
          ),
        ),
      ),
    ),
  );
  
  // describe the personal targets table so Views knows how to join it in
  // this table will be used to join the activity table to the personal message table
  $data['activity_personal_targets']['table'] = array(
    'group' => t('Activity'),
    'join' => array(
      'activity' => array(
        'type' => 'LEFT',
        'left_field' => 'aid',
        'field' => 'aid',
        'extra' => array(
          array(
            'value' => '***CURRENT_USER***', // grab the personal message
            'numeric' => TRUE,
            'field' => 'uid',
          ),
          array(
            'value' => '***CURRENT_LANGUAGE***',
            'field' => 'language',
          ),
        ),
        'table' => 'activity_targets',
      ),
    ),
  );
  
  // add in the activity message table
  $data['activity_messages']['table']['group'] = t('Activity');
  $data['activity_messages']['table']['join'] = array(
    'activity' => array(
      'left_table' => 'activity_targets',
      'left_field' => 'amid',
      'field' => 'amid',
      'type' => 'INNER',
    ),
  );
  
  // create a faux table, that is really activity messages
  $data['activity_personal_messages']['table']['join'] = array(
    'activity' => array(
      'left_table' => 'activity_personal_targets',
      'left_field' => 'amid',
      'field' => 'amid',
      'type' => 'LEFT',
      'table' => 'activity_messages',
    ),
  );
  
  $data['activity_messages']['message'] = array(
    'title' => t('Message'),
    'help' => t('The message may or may not be personalized for the user. Just depends on if the user was involved in the Action'),
    'field' => array(
      'handler' => 'activity_views_handler_field_message',
    ),
  );
  
  // Links to operate upon the activity records
  $data['activity']['delete_activity'] = array(
    'field' => array(
      'title' => t('Delete link'),
      'help' => t('Provide a simple link to delete the activity.'),
      'handler' => 'activity_views_handler_field_activity_link_delete',
      'real field' => 'aid',
    ),
  );
  
  // Relationships to other base tables
  $data['activity']['nid']['relationship'] = array(
    'group' => t('Activity'),
    'title' => t('Node'),
    'help' => t('Join the node table to the Activity table'),
    'handler' => 'views_handler_relationship',
    'base' => 'node',
    'base field' => 'nid',
    'label' => t('Node'),
  );
  $data['activity']['uid']['relationship'] = array(
    'group' => t('Activity'),
    'title' => t('User'),
    'help' => t('Join the users table to the Activity table'),
    'handler' => 'views_handler_relationship',
    'base' => 'users',
    'base field' => 'uid',
    'label' => t('User'),
  );
  return $data;
}

/**
 * Implementation of hook_views_handlers().
 */
function activity_views_handlers() {
  return array(
    // Field Handlers.
    'activity_views_handler_field_message',
    'activity_views_handler_field_activity_link_delete',
    'activity_views_handler_field_type',

    // Filter handlers.
    'activity_views_handler_filter_access',
    'activity_views_handler_filter_uid_access',
    'activity_views_handler_filter_actions_id',

    // Argument Handlers.
    'activity_views_handler_argument_activity_user',
  );
}

/**
 * Implementation of hook_views_query_alter().
 */
function activity_views_query_alter(&$view, &$query) {
  if ($query->get_table_info('activity_access')) {
    $query->set_distinct();
  }
  if ($query->get_table_info('activity')) {
    $bypass_node_access = FALSE;

    // If $GLOBALS['user'] can bypass node access, or there are no node access
    // modules or $GLOBALS['user'] has a global view grant (i.e., a view grant
    // for node ID 0), we don't need to alter the query.
    if (user_access('bypass node access', $GLOBALS['user'])) {
      $bypass_node_access = TRUE;
    }
    if (!count(module_implements('node_grants'))) {
      $bypass_node_access = TRUE;
    }
    if (node_access_view_all_nodes($GLOBALS['user'])) {
      $bypass_node_access = TRUE;
    }

    if (!$bypass_node_access) {
      $query->set_distinct();

      // Create a relationship to node_access so that it can be used properly.
      $def = array(
        'table' => 'node_access',
        'field' => 'nid',
        'left_table' => 'activity',
        'left_field' => 'nid',
      );
      $join = new views_join();
      $join->definition = $def;
      $join->construct();
      $join->adjusted = TRUE;

      $table = $query->add_relationship('activity_node_access', $join, 'node_access');

      $grants = db_or();
      foreach (node_access_grants('view') as $realm => $gids) {
        foreach ($gids as $gid) {
          $grants->condition(db_and()
            ->condition($table . '.gid', $gid)
            ->condition($table . '.realm', $realm));
        }
      }

      // Make it so the node_access is required when nid is there not required
      // otherwise.
      $final = db_or()
        ->condition($table . '.nid', 'NULL')
        ->condition($grants);
      $query->add_where('AND', $final);
    }
  }
}