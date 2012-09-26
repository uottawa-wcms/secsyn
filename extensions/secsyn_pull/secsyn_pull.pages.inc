<?php

function secsyn_pull_form($form, &$form_state) {
  $form = array();
  secsyn_pull_form_first($form, $form_state);
  secsyn_pull_form_second($form, $form_state);
  secsyn_pull_form_third($form, $form_state);
  return $form;
}

function secsyn_pull_form_submit($form, &$form_state) {
  if ($form_state['values']['stage'] == 3) {
    module_load_include('inc', 'secsyn_pull', 'services/secsyn_pull.queue');
    $uuids = array();
    foreach ($form_state['values']['content_select'] as $key => $val) {
      if ((!empty($val)) && $val == $key) {
        $uuids[] = $key;
      }
    }
    $remote_profile = twoway_server_load($form_state['values']['remote_server']);
    $queued = secsyn_pull_service_invoke_queue(
        $remote_profile['server_connection'],
        $form_state['values']['filter'],
        $uuids,
        !empty($form_state['values']['is_urgent'])
    );
    drupal_set_message(t('%total item(s) queued on remote server to be syndicated', array(
      '%total' => $queued,
    )));
    $form_state['redirect'] = 'admin/content/syndication';
  } else {
    $form_state['rebuild'] = TRUE;
  }
}

function secsyn_pull_form_third(&$form, &$form_state) {
  if (!empty($form_state['values']['stage'])) {
    if ($form_state['values']['stage'] == 2) {
      module_load_include('inc', 'secsyn_pull', 'services/secsyn_pull.filter_data');
      $filter_data = secsyn_pull_service_request_filter_data(
          $form_state['values']['remote_server'],
          $form_state['values']['filter']
      );
      $form['stage'] = array(
        '#type' => 'hidden',
        '#value' => 3,
      );
      if (count($filter_data)) {
        $header = array(
          'title' => t('Title'),
          'preview' => t('Preview'),
        );
        $rows = array();
        foreach ($filter_data as $data) {
          $rows[$data['uuid']] = array(
            'title' => $data['label'],
            'preview' => drupal_render($data['preview']),
          );
        }
        $form['content_select'] = array(
          '#type' => 'tableselect',
          '#header' => $header,
          '#options' => $rows,
          '#empty_value' => t('No content was found on the remote server'),
        );
        if (user_access('urgent syndication')) {
          $form['is_urgent'] = array(
            '#type' => 'checkbox',
            '#title' => t('Urgent syndication'),
          );
        }
        $form['next'] = array(
          '#type' => 'submit',
          '#value' => t('Pull Items'),
        );
      } else {
        drupal_set_message(t('No content was found on the remote server'), 'warning');
      }
    }
  }
}

function secsyn_pull_form_second(&$form, &$form_state) {
  if (!empty($form_state['values']['stage'])) {
    if ($form_state['values']['stage'] == 1) {
      module_load_include('inc', 'secsyn_pull', 'services/secsyn_pull.filter_list');
      $filter_list = secsyn_pull_service_request_filter_list($form_state['values']['remote_server']);
      $form['stage'] = array(
        '#type' => 'hidden',
        '#value' => 2,
      );
      if (count($filter_list)) {
        $form_filter_list = array();
        foreach ($filter_list as $filter) {
          $form_filter_list[$filter['profile_name']] = $filter['title'];
        }
        $form['filter'] = array(
          '#type' => 'select',
          '#options' => $form_filter_list,
          '#empty_value' => '',
          '#empty_option' => t('Select one...'),
          '#default_value' => '',
          '#title' => t('Filter'),
          '#required' => TRUE,
        );
        $form['next'] = array(
          '#type' => 'submit',
          '#value' => t('Next'),
        );
      } else {
        drupal_set_message(t('There are no filters defined on the remote server. Please request that one be made.'), 'warning');
      }

    } else {
      $form['filter'] = array(
        '#type' => 'hidden',
        '#value' => $form_state['values']['filter'],
      );
    }
  }
}

function secsyn_pull_form_first(&$form, &$form_state) {
  if (empty($form_state['values']['stage'])) {
    $form['remote_server'] = array(
      '#type' => 'select',
      '#options' => twoway_server_list(),
      '#empty_value' => '',
      '#empty_option' => t('Select one...'),
      '#default_value' => '',
      '#title' => t('Remote Server'),
        '#required' => TRUE,
    );
    $form['stage'] = array(
      '#type' => 'hidden',
      '#value' => 1,
    );
    $form['next'] = array(
      '#type' => 'submit',
      '#value' => t('Next'),
    );
  } else {
    $form['remote_server'] = array(
      '#type' => 'hidden',
      '#value' => $form_state['values']['remote_server'],
    );
  }
}
