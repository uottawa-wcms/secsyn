<?php

function secsyn_nodequeue_syndication_form($form, &$form_state, $queue) {
  $form['all_nodes'] = array(
    '#type' => 'checkbox',
    '#title' => t('Syndicate all nodes'),
    '#description' => t('This will syndicate all related nodes and connect them to the node queue as well'),
    '#default_value' => TRUE,
  );
  $form_state['queue'] = $queue;
  _secsyn_common_fields($form, $queue, 'SecureSyndicationNodeQueueProfile');
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Syndicate'),
  );
  return $form;
}

function secsyn_nodequeue_syndication_form_submit($form, &$form_state) {
  _secsyn_common_fields_submit('SecureSyndicationNodeQueueProfile', $form_state['queue'], $form_state);
  if ($form_state['values']['all_nodes']) {
    $entries = db_select('nodequeue_nodes', 'nqn')
        ->fields('nqn')
        ->condition('qid', $form_state['queue']->qid)
        ->execute();
    foreach ($entries as $queue_entry) {
      _secsyn_common_fields_submit('SecureSyndicationNodeQueueEntryProfile', $queue_entry, $form_state, FALSE);    }
  }
  drupal_set_message(t('The nodequeue has been queued for syndication'));
  $form_state['redirect'] = 'admin/structure/nodequeue/' . $form_state['queue']->qid;

}

function secsyn_nodequeue_secsyn_entity_children($entity) {
  $children = array();
  if ($entity->entity_type == 'node') {
    $queues = db_select('nodequeue_nodes', 'nqn')
        ->fields('nqn')
        ->condition('nid', $entity->nid)
        ->execute();
    foreach ($queues as $queue) {
      $children[] = array(
        'object' => $queue,
        'profile' => 'SecureSyndicationNodeQueueEntryProfile',
      );
    }
  }
  return $children;
}