<?php

function secsyn_auto_summary_page() {
  $header = array(
    'title' => t('Title'),
    'filter' => t('Filter'),
    'last_executed' => t('Last Execution'),
    'next_execution' => t('Next Execution'),
    'ops' => t('Operations'),
  );
  $rows = array();
  if (user_access('urgent syndication')) {
    $rs = db_query('SELECT * FROM {secsyn_auto_config} ORDER BY title');
  } else {
    $rs = db_query('SELECT * FROM {secsyn_auto_config} WHERE urgent = \'N\' ORDER BY title');
  }
  foreach ($rs as $result) {
    $row = array();
    $row['title'] = $result->title;
    $profile = db_query('SELECT * FROM {secsyn_filters_profile} WHERE machine_name = :machine', array(
      ':machine' => $result->filter_name
    ))->fetchObject();
    $row['filter'] = l($profile->title, 'admin/content/syndication/filters/' . $profile->machine_name . '/edit');
    if (!empty($result->last_execution)) {
      $row['last_executed'] = format_date($result->last_execution);
      $next = $result->last_execution + $result->execution_min_delay;
      if ($next < time()) {
        $row['next_execution'] = t('ASAP');
      } else {
        $row['next_execution'] = format_date($next);
      }
    } else {
      $row['last_executed'] = t('Never');
      $row['next_execution'] = t('ASAP');
    }
    $ops = array(
      l('edit', 'admin/content/syndication/auto/' . $result->machine_name . '/edit'),
      l('delete', 'admin/content/syndication/auto/' . $result->machine_name . '/delete'),
    );
    $row['ops'] = implode(' | ', $ops);
    $rows[] = $row;
  }
  return theme('table', array(
    'header' => $header,
    'rows' => $rows,
  ));
}

function secsyn_auto_syndicator_form_submit($form, &$form_state) {
  $syndicator = array(
    'title' => $form_state['values']['title'],
    'machine_name' => $form_state['values']['machine_name'],
    'filter_name' => $form_state['values']['filter_name'],
    'execution_min_delay' => $form_state['values']['execution_delay'],
    'last_execution' => 0,
    'remote_server_machine' => $form_state['values']['remote_host'],
    'enabled' => !empty($form_state['values']['enabled']) ? 'Y' : 'N',
    'urgent' => !empty($form_state['values']['urgent']) ? 'Y' : 'N',
  );
  db_insert('secsyn_auto_config')
    ->fields($syndicator)
    ->execute();
  drupal_set_message(t('Auto syndicator created'));
  $form_state['redirect'] = 'admin/content/syndication/auto';
}

function secsyn_auto_syndicator_form($form, &$form_state, $autosyn = NULL) {
  $form = array();
  if (empty($autosyn)) {
    $autosyn = (object) array(
      'title' => '',
      'machine_name' => '',
      'remote_server_machine' => '',
      'filter_name' => '',
      'execution_min_delay' => 60,
      'enabled' => 'N',
      'urgent' => 'N',
    );
  }
  $form['title'] = array(
    '#title' => t('Syndicator Title'),
    '#required' => TRUE,
    '#type' => 'textfield',
    '#maxlength' => 256,
    '#default_value' => $autosyn->title,
  );
  if (empty($autosyn->machine_name)) {
    $form['machine_name'] = array(
      '#title' => t('Machine Name'),
      '#required' => TRUE,
      '#type' => 'machine_name',
      '#maxlength' => 64,
      '#machine_name' => array(
        'exists' => '_secsyn_auto_syndicator_name_check',
        'source' => array('title'),
      ),
    );
  } else {
    $form['machine_name'] = array(
      '#value' => $autosyn->machine_name,
      '#type' => 'value',
    );
  }
  $form['remote_host'] = array(
    '#title' => t('Remote Connection'),
    '#required' => TRUE,
    '#type' => 'select',
    '#options' => twoway_server_list(),
    '#empty_value' => '',
    '#empty_option' => t('Select one...'),
    '#default_value' => $autosyn->remote_server_machine,
  );
  $filters = array();
  $rs = db_query('SELECT * FROM {secsyn_filters_profile} ORDER BY title');
  foreach ($rs as $row) {
    $filters[$row->machine_name] = $row->title;
  }
  $form['filter_name'] = array(
    '#title' => t('Filter'),
    '#required' => TRUE,
    '#type' => 'select',
    '#options' => $filters,
    '#empty_value' => '',
    '#empty_option' => t('Select one...'),
    '#default_value' => $autosyn->filter_name,
  );
  $form['execution_delay'] = array(
    '#title' => t('Frequency'),
    '#required' => TRUE,
    '#type' => 'textfield',
    '#maxlength' => 10,
    '#description' => t('Time (in seconds) to wait before running this syndicator again'),
    '#default_value' => $autosyn->execution_min_delay,
  );
  $form['enabled'] = array(
    '#title' => t('Enable auto-syndicator'),
    '#type' => 'checkbox',
    '#default_value' => $autosyn->enabled == 'Y' ? TRUE : FALSE,
  );
  if (user_access('urgent syndication')) {
    $form['urgent'] = array(
      '#title' => t('Queue as urgent'),
      '#type' => 'checkbox',
      '#description' => t('Urgent syndications are done immediately'),
      '#default_value' => $autosyn->urgent == 'Y' ? TRUE : FALSE,
    );
  }
  $form['submit'] = array(
    '#value' => t('Save'),
    '#type' => 'submit',
  );
  return $form;
}

function secsyn_auto_delete_syndicator_form($form, &$form_state, $autosync) {
  $form['autosync'] = array(
    '#type' => 'value',
    '#value' => $autosync->machine_name,
  );
  return confirm_form($form,
      t('Are you sure you want to delete this syndicator?'),
      'admin/content/syndication/auto',
      t('This action cannot be undone'),
      t('Delete')
  );
}

function secsyn_auto_delete_syndicator_form_submit($form, &$form_state) {
  $mach = $form_state['values']['autosync'];
  db_delete('secsyn_auto_config')
    ->condition('machine_name', $mach)
    ->execute();
  drupal_set_message(t('Syndicator deleted'));
  $form_state['redirect'] = 'admin/content/syndication/auto';
}