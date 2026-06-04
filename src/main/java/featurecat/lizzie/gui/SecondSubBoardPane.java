package featurecat.lizzie.gui;

import static java.awt.image.BufferedImage.TYPE_INT_ARGB;
import static java.lang.Math.max;

import featurecat.lizzie.Lizzie;
import featurecat.lizzie.rules.Board;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.RenderingHints;
import java.awt.event.MouseEvent;
import java.awt.image.BufferedImage;
import java.util.ArrayList;

public class SecondSubBoardPane extends LizziePane {

  private final BoardRenderer boardRenderer;
  private BufferedImage cachedImage;

  public SecondSubBoardPane(LizzieMain owner) {
    super(owner);
    boardRenderer = new BoardRenderer(false);
  }

  @Override
  protected void checkRightClick(MouseEvent e) {
    // no right-click menu on this pane
  }

  @Override
  protected void paintComponent(Graphics g0) {
    super.paintComponent(g0);

    int width = getWidth();
    int height = getHeight();
    if (width <= 0 || height <= 0) return;

    cachedImage = new BufferedImage(width, height, TYPE_INT_ARGB);
    Graphics2D g = (Graphics2D) cachedImage.getGraphics();
    g.setRenderingHint(RenderingHints.KEY_RENDERING, RenderingHints.VALUE_RENDER_QUALITY);

    if (Lizzie.leelaz != null && Lizzie.config.showSecondSubBoard) {
      try {
        boardRenderer.setLocation(0, 0);
        if (boardParams == null || width != boardParams[0] || height != boardParams[3]) {
          boardParams =
              boardRenderer.availableLength(
                  max(width, Board.boardWidth + 5),
                  max(height, Board.boardHeight + 5),
                  false,
                  false);
        }
        boardRenderer.setBoardParam(boardParams);
        boardRenderer.draw(g);
      } catch (Exception e) {
        // Can happen when no space remains.
      }
    }

    g.dispose();
    Graphics2D bsGraphics = (Graphics2D) g0;
    bsGraphics.setRenderingHint(RenderingHints.KEY_RENDERING, RenderingHints.VALUE_RENDER_QUALITY);
    bsGraphics.drawImage(cachedImage, 0, 0, null);
    bsGraphics.dispose();
  }

  public void resetImages() {
    boardRenderer.resetImages();
  }

  public void removeEstimateRect() {
    boardRenderer.removeEstimateRect();
  }

  public void drawEstimateRect(ArrayList<Double> estimateArray, boolean isZen) {
    boardRenderer.drawEstimateRect(estimateArray, isZen, true);
  }

  public void clearBeforeMove() {
    boardRenderer.clearBeforeMove();
  }
}
