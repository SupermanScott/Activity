// $Id: $
(function ($) {
  /**
   * Behavior to attach to the show all class.
   */
  Drupal.behaviors.activityCommentsShowAll = {
    attach: function (context) {
      $('.activity-comment-show-all:not(.activity-comment-show-all-processed)').each(function (i) {
	$(this).click(function() {
	  // remove the hide comments class so everything is shown below.
	  $(this).parents('.activity-comments-hide-comments').removeClass('activity-comments-hide-comments');
	  return false;
	});
      }).addClass('activity-comment-show-all-processed');
    }
  };

  /**
   * Handles default text and enter key on the comment textfield.
   */
  Drupal.behaviors.activityCommentTextEvents = {
    attach: function (context) {
      $('textarea.activity-comment-text:not(.activity-comment-processed)').each(function (i) {
	  // Focus on the form area, hide the default text.
	  $(this).focus(function() {
	    $this = $(this);
	    if ($this.val() == Drupal.settings.activity_comments.default_text) {
              $this.val('');
            }
	    $this.parents('.activity-comment-add').removeClass('activity-comment-onerow');
            }).blur(function() {
	      $this = $(this);
	      if ($this.val() == '') {
                $this.val(Drupal.settings.activity_comments.default_text);
              }
	      $this.parents('.activity-comment-add').addClass('activity-comment-onerow');
            }).val(Drupal.settings.activity_comments.default_text)
	      .parents('.activity-comment-add').addClass('activity-comment-onerow');
	  });
      }
  };

  /**
   * Handles the destination for the delete link.
   */
  Drupal.behaviors.activityDestination = {
    attach: function(context) {
      $('.activity-comments-delete a:not(.activity-destination-processed)').each(
	function() {
	  var href = $(this).attr('href').split('?destination=')[0] + '?' + Drupal.settings.activity_comments.destination;
          $(this).attr('href', href).addClass('activity-destination-processed');
        });
    }
  };
})(jQuery);
