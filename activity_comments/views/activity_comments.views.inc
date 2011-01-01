<?php
// $Id: $

/**
 * Implementation of hook_views_data().
 */
function activity_comments_views_data() {
  $data['activity_comments']['table']['group'] = t('Activity comments');
  $data['activity_comments_field']['table']['join'] = array(
    'activity' => array(
      'left_field' => 'aid',
      'field' => 'aid',
    ),
  );

  $data['activity_comments_field']['comment_form'] = array(
    'title' =>  t('Comments'),
    'help' => t('Comments to activities.'),
    'group' => t('Activity Comments'),
    'field' => array(
      'handler' => 'activity_comments_handler_field_comments',
    ),
  );
  
  $data['activity_comments_stats']['table']['group'] = t('Activity comments');

  $data['activity_comments_stats']['table']['join'] = array(
    'activity' => array(
      'left_field' => 'aid',
      'field' => 'aid',
    ),
  );
  
  $data['activity_comments_stats']['changed'] = array(
    'title' => t('Changed time'),
    'help' => t('The time when last activity comment was added'),
    'field' => array(
      'handler' => 'views_handler_field_date',
      'click sortable' => TRUE,
    ),
    'sort' => array(
      'handler' => 'views_handler_sort_date',
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_date',
    ),
  );
  
  $data['activity_comments_stats']['comment_count'] = array(
    'title' => t('Comment count'),
    'help' => t('The comment count on an activity'),
    'field' => array(
      'handler' => 'views_handler_field_numeric',
      'click sortable' => TRUE,
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_numeric',
    ),
  );

  return $data;
}

/**
 * Implementation of hook_views_handlers().
 */
function activity_comments_views_handlers() {
  return array(
    'activity_comments_handler_field_comments'
  );
}

/**
 * Activity comments comment form field handler.
 */
class activity_comments_handler_field_comments extends views_handler_field {
  function init(&$view, $options) {
    parent::init($view, $options);
    $this->additional_fields['aid'] = array('table' => 'activity', 'field' => 'aid');
    $this->additional_fields['actions_id'] = array('table' => 'activity', 'field' => 'actions_id');
  }
  
  function query() {
    $this->add_additional_fields();
  }
  
  function render($values) {
    return render(drupal_get_form('activity_comments_form', $values->{$this->aliases['aid']}, $this->options['limit'], $this->options['order']));
  }
  
  function option_definition() {
    $options = parent::option_definition();

    $options['order'] = array(
      'default' => 'ASC',
      'translatable' => FALSE,
    );
    $options['limit'] = array(
      'default' => '2',
      'translatable' => FALSE,
    );
    $options['templates'] = array(
      'default' => array(),
      'translatable' => FALSE,
    );

    return $options;
  }
  
  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);
    $form['order'] = array(
      '#type' => 'select',
      '#title' => t('Order comments by'),
      '#default_value' => $this->options['order'],
      '#options' => array(
        'ASC' => t('Created time - ascending'),
        'DESC' => t('Created time - descending')
      ),
    );
    $form['limit'] = array(
      '#type' => 'textfield',
      '#title' => t('Comments number limit'),
      '#size' => 5,      
      '#default_value' => $this->options['limit'],
    );

    $results = db_query("SELECT aid, label FROM {actions} WHERE type = 'activity'")
      ->fetchAllAssoc('aid');
 
    foreach ($results as $actions_id => $row) {
      $options[$actions_id] = check_plain($row->label);
    }

    $form['templates'] = array(
      '#title' => t('Templates'),
      '#type' => 'checkboxes',
      '#options' => $options,
      '#description' => t('Select the templates that users can comment on'),
      '#default_value' => $this->options['templates'],
      '#required' => TRUE,
    );
  }

  function options_validate(&$form, &$form_state) {
    $form_state['values']['templates'] = array_filter($form_state['values']['templates']);
  }
}